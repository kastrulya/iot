#include <Arduino.h>
#include <Button.cpp>
#include <BlinkingLed.cpp>
#include <Base.h>
#include <Event.h>

typedef vl::Func<void(void)> Callback;

enum State {
    T_STARTED = 0,
    S_STARTED,
    T_STOPPED,
    S_STOPPED,
    __TOTAL
};

class ButtonFSM : public Updatable {

    const unsigned int BTN_DELAY = 300;

    mutable State state;
    Button button;
    BlinkingLed blinkingLed;
    Timer *timer;

public:

    void onButtonClick();
    void setState(State s);

    ButtonFSM() :
            state(T_STARTED),
            button(8),
            blinkingLed(13, 100) {
        Component::registerComponent(this);
        eventBus->subscribe(Event::E_BUTTON_CLICK, Callback(this, &ButtonFSM::onButtonClick));
    }

    void update() {
        switch (state) {
            case T_STARTED:
//                Serial.println( "T_STARTED change state ");
                blinkingLed.start();
                state = S_STARTED;
//                timer = new Timer(Callback(this, &ButtonFSM::onButtonClick), 2000);
                break;
            case T_STOPPED:
//                Serial.println( "T_STOPPED change state ");
                blinkingLed.stop();
                state = S_STOPPED;
                break;
        }
    }
};

void ButtonFSM::onButtonClick() {
    switch (state) {
        case T_STARTED:
        case S_STARTED:
            setState(T_STOPPED);
            break;
        case T_STOPPED:
        case S_STOPPED:
            setState(T_STARTED);
            break;
    }
}

void ButtonFSM::setState(State s) {
//    delete this->timer;
    this->state = s;
}
