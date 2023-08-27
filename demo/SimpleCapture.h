#pragma once

class SimpleCapture {
public:
  SimpleCapture(
      winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const
          &device,
      winrt::Windows::Graphics::Capture::GraphicsCaptureItem const &item);
  ~SimpleCapture() { Close(); }

  void StartCapture();
  winrt::Windows::UI::Composition::ICompositionSurface
  CreateSurface(winrt::Windows::UI::Composition::Compositor const &compositor);

  void Close();

private:
  void OnFrameArrived(
      winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const
          &sender,
      winrt::Windows::Foundation::IInspectable const &args);

  void CheckClosed() {
    if (m_closed.load() == true) {
      throw winrt::hresult_error(RO_E_CLOSED);
    }
  }

  HRESULT
  CreateMappedTexture(winrt::com_ptr<ID3D11Texture2D> src_texture,
                      UINT width = 0, UINT height = 0);

private:
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{nullptr};
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{
      nullptr};
  winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{nullptr};
  winrt::Windows::Graphics::SizeInt32 m_lastSize;

  winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{
      nullptr};
  winrt::com_ptr<IDXGISwapChain1> m_swapChain{nullptr};
  winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{nullptr};
  winrt::com_ptr<ID3D11Texture2D> m_mappedTexture{nullptr};

  std::atomic<bool> m_closed = false;
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::
      FrameArrived_revoker m_frameArrived;
};