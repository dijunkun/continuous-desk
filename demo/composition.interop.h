#pragma once
#include <d2d1_1.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.UI.Composition.h>

inline auto CreateCompositionGraphicsDevice(
    winrt::Windows::UI::Composition::Compositor const &compositor,
    ::IUnknown *device) {
  winrt::Windows::UI::Composition::CompositionGraphicsDevice graphicsDevice{
      nullptr};
  auto compositorInterop =
      compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
  winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionGraphicsDevice>
      graphicsInterop;
  winrt::check_hresult(
      compositorInterop->CreateGraphicsDevice(device, graphicsInterop.put()));
  winrt::check_hresult(graphicsInterop->QueryInterface(
      winrt::guid_of<
          winrt::Windows::UI::Composition::CompositionGraphicsDevice>(),
      reinterpret_cast<void **>(winrt::put_abi(graphicsDevice))));
  return graphicsDevice;
}

inline void ResizeSurface(
    winrt::Windows::UI::Composition::CompositionDrawingSurface const &surface,
    winrt::Windows::Foundation::Size const &size) {
  auto surfaceInterop = surface.as<
      ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
  SIZE newSize = {};
  newSize.cx = static_cast<LONG>(std::round(size.Width));
  newSize.cy = static_cast<LONG>(std::round(size.Height));
  winrt::check_hresult(surfaceInterop->Resize(newSize));
}

inline auto SurfaceBeginDraw(
    winrt::Windows::UI::Composition::CompositionDrawingSurface const &surface) {
  auto surfaceInterop = surface.as<
      ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
  winrt::com_ptr<ID2D1DeviceContext> context;
  POINT offset = {};
  winrt::check_hresult(surfaceInterop->BeginDraw(
      nullptr, __uuidof(ID2D1DeviceContext), context.put_void(), &offset));
  context->SetTransform(
      D2D1::Matrix3x2F::Translation((FLOAT)offset.x, (FLOAT)offset.y));
  return context;
}

inline void SurfaceEndDraw(
    winrt::Windows::UI::Composition::CompositionDrawingSurface const &surface) {
  auto surfaceInterop = surface.as<
      ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
  winrt::check_hresult(surfaceInterop->EndDraw());
}

inline auto CreateCompositionSurfaceForSwapChain(
    winrt::Windows::UI::Composition::Compositor const &compositor,
    ::IUnknown *swapChain) {
  winrt::Windows::UI::Composition::ICompositionSurface surface{nullptr};
  auto compositorInterop =
      compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
  winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionSurface>
      surfaceInterop;
  winrt::check_hresult(compositorInterop->CreateCompositionSurfaceForSwapChain(
      swapChain, surfaceInterop.put()));
  winrt::check_hresult(surfaceInterop->QueryInterface(
      winrt::guid_of<winrt::Windows::UI::Composition::ICompositionSurface>(),
      reinterpret_cast<void **>(winrt::put_abi(surface))));
  return surface;
}