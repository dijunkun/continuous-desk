#include "pch.h"

#include <winrt/Windows.Foundation.Metadata.h>

bool wgc_is_supported() {
  try {
    /* no contract for IGraphicsCaptureItemInterop, verify 10.0.18362.0 */
    return winrt::Windows::Foundation::Metadata::ApiInformation::
        IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
  } catch (const winrt::hresult_error &) {
    return false;
  } catch (...) {
    return false;
  }
}

am::wgc_session *wgc_create_session() { return new am::wgc_session_impl(); }