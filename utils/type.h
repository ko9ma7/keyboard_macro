// type.h
#ifndef KEY_MACRO_TYPE_H
#define KEY_MACRO_TYPE_H

namespace KeyMacro {
    struct KeyEvent {
        uint64_t delay;
        std::vector<unsigned char> data;
    };
}

#endif // KEY_MACRO_TYPE_H
