#pragma once

#include <Windows.h>

#include "export.h"
#include "record_desktop.h"

namespace am {
class record_desktop_wgc : public record_desktop,
                           public wgc_session::wgc_session_observer {
  class wgc_session_module {
    using func_type_is_supported = bool (*)();
    using func_type_create_session = wgc_session *(*)();

   public:
    wgc_session_module() {
      module_ = ::LoadLibraryA("WGC.dll");
      if (module_) {
        func_is_supported_ = (func_type_is_supported)::GetProcAddress(
            module_, "wgc_is_supported");
        func_create_session_ = (func_type_create_session)::GetProcAddress(
            module_, "wgc_create_session");
      }
    }
    ~wgc_session_module() {
      if (module_) ::FreeModule(module_);
    }

    bool is_supported() const {
      return func_create_session_ && func_is_supported_();
    }

    wgc_session *create_session() const {
      if (!func_create_session_) return nullptr;

      return func_create_session_();
    }

   private:
    HMODULE module_ = nullptr;
    func_type_is_supported func_is_supported_ = nullptr;
    func_type_create_session func_create_session_ = nullptr;
  };

 public:
  record_desktop_wgc();
  ~record_desktop_wgc();

  int init(const RECORD_DESKTOP_RECT &rect, const int fps) override;

  int start() override;
  int pause() override;
  int resume() override;
  int stop() override;

  void on_frame(const wgc_session::wgc_session_frame &frame) override;

 protected:
  void clean_up() override;

 private:
  wgc_session *session_ = nullptr;
  wgc_session_module module_;
};
}  // namespace am