#include "bluetooth_registry.h"

void BluetoothRegistry::registerAllDevices() {
    GError* error = NULL;
    GDBusProxy* proxy = g_dbus_proxy_new_sync(bus,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL, /* GDBusInterfaceInfo */
                                                "org.bluez",
                                                "/",
                                                "org.freedesktop.DBus.ObjectManager",
                                                NULL, /* GCancellable */
                                                &error);

    if (!proxy) {
        std::cerr << "Failed to create proxy for Object Manager: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    GVariant* result = g_dbus_proxy_call_sync(proxy,
                                                "GetManagedObjects",
                                                NULL, /* parameters */
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1, /* timeout_msec */
                                                NULL, /* GCancellable */
                                                &error);

    if (!result) {
        std::cerr << "Error calling GetManagedObjects: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(proxy);
        return;
    }

    GVariantIter* iter;
    gchar* object_path;
    GVariant* interfaces_and_properties;

    g_variant_get(result, "(a{oa{sa{sv}}})", &iter);
    while (g_variant_iter_loop(iter, "{o@a{sa{sv}}}", &object_path, &interfaces_and_properties)) {
        std::cout<<"register device: "<<object_path<<'\n';
        addDevice(object_path, interfaces_and_properties);
    }

    g_variant_iter_free(iter);
    g_variant_unref(result);
    g_object_unref(proxy);
}

void BluetoothRegistry::addDevice(const gchar* devicePath, GVariant* interfaces_and_properties) {
    // 속성 조회 전에 로그를 출력하여 어떤 인터페이스와 속성이 있는지 확인
    GVariantIter iter;
    gchar *key;
    GVariant *value;
    g_variant_iter_init(&iter, interfaces_and_properties);
    while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
        std::cout << "Found property: " << key << std::endl;
        g_free(key);
        g_variant_unref(value);
    }

    // 소켓 경로 속성 조회
    GVariant* ctrl_path_prop = g_variant_lookup_value(interfaces_and_properties, "SocketPathCtrl", G_VARIANT_TYPE_STRING);
    GVariant* intr_path_prop = g_variant_lookup_value(interfaces_and_properties, "SocketPathIntr", G_VARIANT_TYPE_STRING);

    if (!ctrl_path_prop || !intr_path_prop) {
        std::cerr << "Socket paths not found for device: " << devicePath << std::endl;
        return;
    }

    std::string ctrl_path = g_variant_get_string(ctrl_path_prop, NULL);
    std::string intr_path = g_variant_get_string(intr_path_prop, NULL);

    auto device = std::make_shared<BluetoothDevice>(ctrl_path, intr_path);
    devices.push_back(device);
    std::cout << "Device added: " << devicePath << " with control path: " << ctrl_path << " and interrupt path: " << intr_path << std::endl;

    g_variant_unref(ctrl_path_prop);
    g_variant_unref(intr_path_prop);
}


int main() {
    GError* error = nullptr;
    GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (error != nullptr) {
        std::cerr << "Failed to connect to D-Bus system bus: " << error->message << std::endl;
        g_error_free(error);
        return 1;
    }

    BluetoothRegistry reg(connection);

    reg.registerAllDevices();
} 