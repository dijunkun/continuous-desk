#include "pch.h"

#include <functional>
#include <memory>

#define CHECK_INIT                                                             \
  if (!is_initialized_)                                                        \
  return AM_ERROR::AE_NEED_INIT

#define CHECK_CLOSED                                                           \
  if (cleaned_.load() == true) {                                               \
    throw winrt::hresult_error(RO_E_CLOSED);                                   \
  }

extern "C" {
HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(
    ::IDXGIDevice *dxgiDevice, ::IInspectable **graphicsDevice);
}

namespace am {

wgc_session_impl::wgc_session_impl() {}

wgc_session_impl::~wgc_session_impl() {
  stop();
  cleanup();
}

void wgc_session_impl::release() { delete this; }

int wgc_session_impl::initialize(HWND hwnd) {
  std::lock_guard locker(lock_);

  target_.hwnd = hwnd;
  target_.is_window = true;
  return initialize();
}

int wgc_session_impl::initialize(HMONITOR hmonitor) {
  std::lock_guard locker(lock_);

  target_.hmonitor = hmonitor;
  target_.is_window = false;
  return initialize();
}

void wgc_session_impl::register_observer(wgc_session_observer *observer) {
  std::lock_guard locker(lock_);
  observer_ = observer;
}

int wgc_session_impl::start() {
  std::lock_guard locker(lock_);

  if (is_running_)
    return AM_ERROR::AE_NO;

  int error = AM_ERROR::AE_WGC_CREATE_CAPTURER_FAILED;

  CHECK_INIT;
  try {
    if (!capture_session_) {
      auto current_size = capture_item_.Size();
      capture_framepool_ =
          winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::
              CreateFreeThreaded(d3d11_direct_device_,
                                 winrt::Windows::Graphics::DirectX::
                                     DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                 2, current_size);
      capture_session_ = capture_framepool_.CreateCaptureSession(capture_item_);
      capture_frame_size_ = current_size;
      capture_framepool_trigger_ = capture_framepool_.FrameArrived(
          winrt::auto_revoke, {this, &wgc_session_impl::on_frame});
      capture_close_trigger_ = capture_item_.Closed(
          winrt::auto_revoke, {this, &wgc_session_impl::on_closed});
    }

    if (!capture_framepool_)
      throw std::exception();

    is_running_ = true;

    // we do not need to crate a thread to enter a message loop coz we use
    // CreateFreeThreaded instead of Create to create a capture frame pool,
    // we need to test the performance later
    // loop_ = std::thread(std::bind(&wgc_session_impl::message_func, this));

    capture_session_.StartCapture();

    error = AM_ERROR::AE_NO;
  } catch (winrt::hresult_error) {
    return AM_ERROR::AE_WGC_CREATE_CAPTURER_FAILED;
  } catch (...) {
    return AM_ERROR::AE_WGC_CREATE_CAPTURER_FAILED;
  }

  return error;
}

int wgc_session_impl::stop() {
  std::lock_guard locker(lock_);

  CHECK_INIT;

  is_running_ = false;

  if (loop_.joinable())
    loop_.join();

  if (capture_framepool_trigger_)
    capture_framepool_trigger_.revoke();

  if (capture_session_) {
    capture_session_.Close();
    capture_session_ = nullptr;
  }

  return AM_ERROR::AE_NO;
}

int wgc_session_impl::pause() {
  std::lock_guard locker(lock_);

  CHECK_INIT;
  return AM_ERROR::AE_NO;
}

int wgc_session_impl::resume() {
  std::lock_guard locker(lock_);

  CHECK_INIT;
  return AM_ERROR::AE_NO;
}

auto wgc_session_impl::create_d3d11_device() {
  auto create_d3d_device = [](D3D_DRIVER_TYPE const type,
                              winrt::com_ptr<ID3D11Device> &device) {
    WINRT_ASSERT(!device);

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    //#ifdef _DEBUG
    //	flags |= D3D11_CREATE_DEVICE_DEBUG;
    //#endif

    return ::D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0,
                               D3D11_SDK_VERSION, device.put(), nullptr,
                               nullptr);
  };
  auto create_d3d_device_wrapper = [&create_d3d_device]() {
    winrt::com_ptr<ID3D11Device> device;
    HRESULT hr = create_d3d_device(D3D_DRIVER_TYPE_HARDWARE, device);

    if (DXGI_ERROR_UNSUPPORTED == hr) {
      hr = create_d3d_device(D3D_DRIVER_TYPE_WARP, device);
    }

    winrt::check_hresult(hr);
    return device;
  };

  auto d3d_device = create_d3d_device_wrapper();
  auto dxgi_device = d3d_device.as<IDXGIDevice>();

  winrt::com_ptr<::IInspectable> d3d11_device;
  winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(
      dxgi_device.get(), d3d11_device.put()));
  return d3d11_device
      .as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

auto wgc_session_impl::create_capture_item(HWND hwnd) {
  auto activation_factory = winrt::get_activation_factory<
      winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
  auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};
  interop_factory->CreateForWindow(
      hwnd,
      winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
      reinterpret_cast<void **>(winrt::put_abi(item)));
  return item;
}

auto wgc_session_impl::create_capture_item(HMONITOR hmonitor) {
  auto activation_factory = winrt::get_activation_factory<
      winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
  auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};
  interop_factory->CreateForMonitor(
      hmonitor,
      winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
      reinterpret_cast<void **>(winrt::put_abi(item)));
  return item;
}

HRESULT wgc_session_impl::create_mapped_texture(
    winrt::com_ptr<ID3D11Texture2D> src_texture, unsigned int width,
    unsigned int height) {

  D3D11_TEXTURE2D_DESC src_desc;
  src_texture->GetDesc(&src_desc);
  D3D11_TEXTURE2D_DESC map_desc;
  map_desc.Width = width == 0 ? src_desc.Width : width;
  map_desc.Height = height == 0 ? src_desc.Height : height;
  map_desc.MipLevels = src_desc.MipLevels;
  map_desc.ArraySize = src_desc.ArraySize;
  map_desc.Format = src_desc.Format;
  map_desc.SampleDesc = src_desc.SampleDesc;
  map_desc.Usage = D3D11_USAGE_STAGING;
  map_desc.BindFlags = 0;
  map_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  map_desc.MiscFlags = 0;

  auto d3dDevice = get_dxgi_interface<ID3D11Device>(d3d11_direct_device_);

  return d3dDevice->CreateTexture2D(&map_desc, nullptr,
                                    d3d11_texture_mapped_.put());
}

void wgc_session_impl::on_frame(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender,
    winrt::Windows::Foundation::IInspectable const &args) {
  std::lock_guard locker(lock_);

  auto is_new_size = false;

  {
    auto frame = sender.TryGetNextFrame();
    auto frame_size = frame.ContentSize();

    if (frame_size.Width != capture_frame_size_.Width ||
        frame_size.Height != capture_frame_size_.Height) {
      // The thing we have been capturing has changed size.
      // We need to resize our swap chain first, then blit the pixels.
      // After we do that, retire the frame and then recreate our frame pool.
      is_new_size = true;
      capture_frame_size_ = frame_size;
    }

    // copy to mapped texture
    {
      auto frame_captured =
          get_dxgi_interface<ID3D11Texture2D>(frame.Surface());

      if (!d3d11_texture_mapped_ || is_new_size)
        create_mapped_texture(frame_captured);

      d3d11_device_context_->CopyResource(d3d11_texture_mapped_.get(),
                                          frame_captured.get());

      D3D11_MAPPED_SUBRESOURCE map_result;
      HRESULT hr = d3d11_device_context_->Map(
          d3d11_texture_mapped_.get(), 0, D3D11_MAP_READ,
          0 /*coz we use CreateFreeThreaded, so we cant use flags
               D3D11_MAP_FLAG_DO_NOT_WAIT*/
          ,
          &map_result);
      if (FAILED(hr)) {
        OutputDebugStringW(
            (L"map resource failed: " + std::to_wstring(hr)).c_str());
      }

      // copy data from map_result.pData
      if (map_result.pData && observer_) {
        observer_->on_frame(wgc_session_frame{
            static_cast<unsigned int>(frame_size.Width),
            static_cast<unsigned int>(frame_size.Height), map_result.RowPitch,
            const_cast<const unsigned char *>(
                (unsigned char *)map_result.pData)});
      }
#if 0
      if (map_result.pData) {
        static unsigned char *buffer = nullptr;
        if (buffer && is_new_size)
          delete[] buffer;

        if (!buffer)
          buffer = new unsigned char[frame_size.Width * frame_size.Height * 4];

        int dstRowPitch = frame_size.Width * 4;
        for (int h = 0; h < frame_size.Height; h++) {
          memcpy_s(buffer + h * dstRowPitch, dstRowPitch,
                   (BYTE *)map_result.pData + h * map_result.RowPitch,
                   min(map_result.RowPitch, dstRowPitch));
        }

        BITMAPINFOHEADER bi;

        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = frame_size.Width;
        bi.biHeight = frame_size.Height * (-1);
        bi.biPlanes = 1;
        bi.biBitCount = 32; // should get from system color bits
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        BITMAPFILEHEADER bf;
        bf.bfType = 0x4d42;
        bf.bfReserved1 = 0;
        bf.bfReserved2 = 0;
        bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bf.bfSize = bf.bfOffBits + frame_size.Width * frame_size.Height * 4;

        FILE *fp = nullptr;

        fopen_s(&fp, ".\\save.bmp", "wb+");

        fwrite(&bf, 1, sizeof(bf), fp);
        fwrite(&bi, 1, sizeof(bi), fp);
        fwrite(buffer, 1, frame_size.Width * frame_size.Height * 4, fp);

        fflush(fp);
        fclose(fp);
      }
#endif

      d3d11_device_context_->Unmap(d3d11_texture_mapped_.get(), 0);
    }
  }

  if (is_new_size) {
    capture_framepool_.Recreate(d3d11_direct_device_,
                                winrt::Windows::Graphics::DirectX::
                                    DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                2, capture_frame_size_);
  }
}

void wgc_session_impl::on_closed(
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem const &,
    winrt::Windows::Foundation::IInspectable const &) {
  OutputDebugStringW(L"wgc_session_impl::on_closed");
}

int wgc_session_impl::initialize() {
  if (is_initialized_)
    return AM_ERROR::AE_NO;

  if (!(d3d11_direct_device_ = create_d3d11_device()))
    return AM_ERROR::AE_D3D_CREATE_DEVICE_FAILED;

  try {
    if (target_.is_window)
      capture_item_ = create_capture_item(target_.hwnd);
    else
      capture_item_ = create_capture_item(target_.hmonitor);

    // Set up
    auto d3d11_device = get_dxgi_interface<ID3D11Device>(d3d11_direct_device_);
    d3d11_device->GetImmediateContext(d3d11_device_context_.put());

  } catch (winrt::hresult_error) {
    return AM_ERROR::AE_WGC_CREATE_CAPTURER_FAILED;
  } catch (...) {
    return AM_ERROR::AE_WGC_CREATE_CAPTURER_FAILED;
  }

  is_initialized_ = true;

  return AM_ERROR::AE_NO;
}

void wgc_session_impl::cleanup() {
  std::lock_guard locker(lock_);

  auto expected = false;
  if (cleaned_.compare_exchange_strong(expected, true)) {
    capture_close_trigger_.revoke();
    capture_framepool_trigger_.revoke();

    if (capture_framepool_)
      capture_framepool_.Close();

    if (capture_session_)
      capture_session_.Close();

    capture_framepool_ = nullptr;
    capture_session_ = nullptr;
    capture_item_ = nullptr;

    is_initialized_ = false;
  }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM w_param,
                            LPARAM l_param) {
  return DefWindowProc(window, message, w_param, l_param);
}

void wgc_session_impl::message_func() {
  const std::wstring kClassName = L"am_fake_window";

  WNDCLASS wc = {};

  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = DefWindowProc;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
  wc.lpszClassName = kClassName.c_str();

  if (!::RegisterClassW(&wc))
    return;

  hwnd_ = ::CreateWindowW(kClassName.c_str(), nullptr, WS_OVERLAPPEDWINDOW, 0,
                          0, 0, 0, nullptr, nullptr, nullptr, nullptr);
  MSG msg;
  while (is_running_) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (!is_running_)
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    Sleep(10);
  }

  ::CloseWindow(hwnd_);
  ::DestroyWindow(hwnd_);
}

} // namespace am