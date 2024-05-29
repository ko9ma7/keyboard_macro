#include "restarter/controller/restart_main_controller.h"

int main() {
    RestartThreadController controller;

    controller.RunServer();

    return 0;
}