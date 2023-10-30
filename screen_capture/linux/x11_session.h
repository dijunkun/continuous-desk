#ifndef _X11_SESSION_H_
#define _X11_SESSION_H_

class X11Session {
 public:
  struct x11_session_frame {
    unsigned int width;
    unsigned int height;
    unsigned int row_pitch;

    const unsigned char *data;
  };

  class x11_session_observer {
   public:
    virtual ~x11_session_observer() {}
    virtual void OnFrame(const x11_session_frame &frame) = 0;
  };

 public:
  virtual void Release() = 0;

  virtual int Initialize() = 0;

  virtual void RegisterObserver(x11_session_observer *observer) = 0;

  virtual int Start() = 0;
  virtual int Stop() = 0;

  virtual int Pause() = 0;
  virtual int Resume() = 0;

 protected:
  virtual ~X11Session(){};
};

#endif