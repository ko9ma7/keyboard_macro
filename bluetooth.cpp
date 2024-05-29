#include "driver/controller/main_controller.h"

int main() {
    ThreadController controller;

    return controller.RunBluetoothThread();
}