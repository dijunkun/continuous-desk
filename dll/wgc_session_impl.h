#pragma once

#include <mutex>
#include <thread>

namespace am {
class wgc_session_impl : public wgc_session {
  struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
      IDirect3DDxgiInterfaceAccess : ::IUnknown {
    virtual HRESULT __stdcall GetInterface(GUID const &id, void **object) = 0;
  };

  struct {
    union {
      HWND hwnd;
      HMONITOR hmonitor;
    };
    bool is_window;
  } target_{0};

public:
  wgc_session_impl();
  ~wgc_session_impl() override;

public:
  void release() override;

  int initialize(HWND hwnd) override;
  int initialize(HMONITOR hmonitor) override;

  void register_observer(wgc_session_observer *observer) override;

  int start() override;
  int stop() override;

  int pause() override;
  int resume() override;

private:
  auto create_d3d11_device();
  auto create_capture_item(HWND hwnd);
  auto create_capture_item(HMONITOR hmonitor);
  template <typename T>
  auto
  get_dxgi_interface(winrt::Windows::Foundation::IInspectable const &object);
  HRESULT create_mapped_texture(winrt::com_ptr<ID3D11Texture2D> src_texture,
                                unsigned int width = 0,
                                unsigned int height = 0);
  void
  on_frame(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const
               &sender,
           winrt::Windows::Foundation::IInspectable const &args);
  void on_closed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const &,
                 winrt::Windows::Foundation::IInspectable const &);

  int initialize();
  void cleanup();

  void message_func();

private:
  std::mutex lock_;
  bool is_initialized_ = false;
  bool is_running_ = false;
  bool is_paused_ = false;

  wgc_session_observer *observer_ = nullptr;

  // wgc
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem capture_item_{nullptr};
  winrt::Windows::Graphics::Capture::GraphicsCaptureSession capture_session_{
      nullptr};
  winrt::Windows::Graphics::SizeInt32 capture_frame_size_;

  winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice
      d3d11_direct_device_{nullptr};
  winrt::com_ptr<ID3D11DeviceContext> d3d11_device_context_{nullptr};
  winrt::com_ptr<ID3D11Texture2D> d3d11_texture_mapped_{nullptr};

  std::atomic<bool> cleaned_ = false;
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool
      capture_framepool_{nullptr};
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::
      FrameArrived_revoker capture_framepool_trigger_;
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker
      capture_close_trigger_;

  // message loop
  std::thread loop_;
  HWND hwnd_ = nullptr;
};

template <typename T>
inline auto wgc_session_impl::get_dxgi_interface(
    winrt::Windows::Foundation::IInspectable const &object) {
  auto access = object.as<IDirect3DDxgiInterfaceAccess>();
  winrt::com_ptr<T> result;
  winrt::check_hresult(
      access->GetInterface(winrt::guid_of<T>(), result.put_void()));
  return result;
}


} // namespace am