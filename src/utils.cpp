#include "kaskad_net-configurator-gtk3/utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glib.h>
#include <glibmm/ustring.h>
#include <gtkmm.h>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

int get_missing_id(const Gtk::TreeModel::Row &servers_row,
                   const ModelColumns &model_columns) {
    int missing_idx = 1;
    auto stations = servers_row->children();
    for (auto iter = stations.begin(); iter != stations.end(); ++iter) {
        if (missing_idx != (*iter)[model_columns.col_id]) {
            return missing_idx;
        }
        ++missing_idx;
    }
    return -1;
}

std::string get_active_ipv4_address() {
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        return "";
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (ifa->ifa_flags & IFF_LOOPBACK)
                continue;

            char ip[INET_ADDRSTRLEN];
            void *addr_ptr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr_ptr, ip, sizeof(ip));

            freeifaddrs(ifaddr);
            return std::string(ip);
        }
    }

    freeifaddrs(ifaddr);
    return "";
}

void resolve_dns_async(Gtk::Window *window, const std::string &dns,
                       Gtk::Entry &entry) {
    auto window_gdk = window->get_window();
    if (window_gdk) {
        window_gdk->set_cursor(Gdk::Cursor::create(Gdk::WATCH));
    }

    auto loading_dialog = std::make_shared<Gtk::MessageDialog>(
        *window, "Определение DNS...", false, Gtk::MESSAGE_INFO,
        Gtk::BUTTONS_NONE, true);
    loading_dialog->set_modal(true);
    loading_dialog->set_resizable(false);
    loading_dialog->show();

    std::thread([window, &entry, dns, loading_dialog]() {
        std::string ip_out;
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo *res = nullptr;

        if (getaddrinfo(dns.c_str(), nullptr, &hints, &res) == 0 &&
            res != nullptr) {
            char ip_str[INET_ADDRSTRLEN]{};
            auto *ipv4 = reinterpret_cast<sockaddr_in *>(res->ai_addr);
            if (inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str))) {
                ip_out = ip_str;
            }
            freeaddrinfo(res);
        }

        Glib::signal_idle().connect_once(
            [window, &entry, ip_out, loading_dialog]() {
                if (window->get_window()) {
                    window->get_window()->set_cursor();
                }
                loading_dialog->hide();

                if (ip_out.empty()) {
                    Gtk::MessageDialog error_dialog(
                        *window, "Ошибка получения IP-адреса DNS-сервера",
                        false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, false);
                    error_dialog.run();
                } else {
                    Gtk::MessageDialog question_dialog(
                        *window,
                        "IP-адрес DNS-сервера: " + ip_out +
                            ". Подставить значение в поле?",
                        false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
                    question_dialog.add_button("Ok", Gtk::RESPONSE_OK);
                    question_dialog.add_button("Отмена", Gtk::RESPONSE_CANCEL);
                    int response = question_dialog.run();
                    if (response == Gtk::RESPONSE_OK) {
                        entry.set_text(ip_out);
                    }
                }
            });
    }).detach();
}

int cp1251_to_utf8(char *out, const char *in, int buflen) {
    static const int table[128] = {
        0x82D0,   0x83D0,   0x9A80E2, 0x93D1,   0x9E80E2, 0xA680E2, 0xA080E2,
        0xA180E2, 0xAC82E2, 0xB080E2, 0x89D0,   0xB980E2, 0x8AD0,   0x8CD0,
        0x8BD0,   0x8FD0,   0x92D1,   0x9880E2, 0x9980E2, 0x9C80E2, 0x9D80E2,
        0xA280E2, 0x9380E2, 0x9480E2, 0,        0xA284E2, 0x99D1,   0xBA80E2,
        0x9AD1,   0x9CD1,   0x9BD1,   0x9FD1,   0xA0C2,   0x8ED0,   0x9ED1,
        0x88D0,   0xA4C2,   0x90D2,   0xA6C2,   0xA7C2,   0x81D0,   0xA9C2,
        0x84D0,   0xABC2,   0xACC2,   0xADC2,   0xAEC2,   0x87D0,   0xB0C2,
        0xB1C2,   0x86D0,   0x96D1,   0x91D2,   0xB5C2,   0xB6C2,   0xB7C2,
        0x91D1,   0x9684E2, 0x94D1,   0xBBC2,   0x98D1,   0x85D0,   0x95D1,
        0x97D1,   0x90D0,   0x91D0,   0x92D0,   0x93D0,   0x94D0,   0x95D0,
        0x96D0,   0x97D0,   0x98D0,   0x99D0,   0x9AD0,   0x9BD0,   0x9CD0,
        0x9DD0,   0x9ED0,   0x9FD0,   0xA0D0,   0xA1D0,   0xA2D0,   0xA3D0,
        0xA4D0,   0xA5D0,   0xA6D0,   0xA7D0,   0xA8D0,   0xA9D0,   0xAAD0,
        0xABD0,   0xACD0,   0xADD0,   0xAED0,   0xAFD0,   0xB0D0,   0xB1D0,
        0xB2D0,   0xB3D0,   0xB4D0,   0xB5D0,   0xB6D0,   0xB7D0,   0xB8D0,
        0xB9D0,   0xBAD0,   0xBBD0,   0xBCD0,   0xBDD0,   0xBED0,   0xBFD0,
        0x80D1,   0x81D1,   0x82D1,   0x83D1,   0x84D1,   0x85D1,   0x86D1,
        0x87D1,   0x88D1,   0x89D1,   0x8AD1,   0x8BD1,   0x8CD1,   0x8DD1,
        0x8ED1,   0x8FD1};

    char *pout = out;
    for (; *in && ((out - pout) < buflen - 1);) {
        if (*in & 0x80) {
            int v = table[(int)(0x7f & *in++)];
            if (!v)
                continue;
            *out++ = (char)v;
            *out++ = (char)(v >> 8);
            if (v >>= 16)
                *out++ = (char)v;
        } else {
            *out++ = *in++;
        }
    }
    *out = 0;
    return (out - pout);
}

void cp1251_to_utf8(const std::string &s, std::string &out) {

    out.resize(s.length() * 2);

    int sz = cp1251_to_utf8(out.data(), s.c_str(), out.length());

    out.resize(sz);
}

std::string cp1251_to_utf8(const std::string &s) {
    std::string out;
    cp1251_to_utf8(s, out);
    return out;
}

bool parse_config(const std::string &config_path, MainSettings &main_settings,
                  Gtk::TreeModel::Row &servers_row,
                  Glib::RefPtr<Gtk::TreeStore> &tree_store,
                  const ModelColumns &model_columns, int &next_station_id,
                  std::string &errors) {
    Glib::KeyFile key_file;
    std::string utf8_data;
    std::ifstream file(config_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл конфигурации по пути "
                  << config_path << std::endl;
        return false;
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    std::string raw_data(buffer.data(), buffer.size());
    if (g_utf8_validate(raw_data.c_str(), static_cast<gssize>(raw_data.size()),
                        nullptr)) {
        utf8_data = raw_data;
    } else {
        utf8_data = cp1251_to_utf8(raw_data);
    }
    try {
        key_file.load_from_data(utf8_data);
    } catch (const Glib::Error &e) {
        std::cerr << "Не удалось прочитать файл конфигурации по пути "
                  << config_path << ": " << e.what() << std::endl;
        return false;
    }
    if (key_file.has_key("Main", "LocalStationID")) {
        main_settings.local_station_id =
            key_file.get_integer("Main", "LocalStationID");
    }

    auto groups = key_file.get_groups();
    auto append_station = [&](const std::string &group_name,
                              std::string &errors) {
        Gtk::TreeModel::Row new_row =
            *(tree_store->append(servers_row.children()));
        new_row[model_columns.col_id] = next_station_id++;
        new_row[model_columns.col_name] =
            key_file.has_key(group_name, "Name")
                ? Glib::ustring(key_file.get_string(group_name, "Name"))
                : Glib::ustring(group_name);
        new_row[model_columns.col_comments] =
            key_file.has_key(group_name, "Comments")
                ? Glib::ustring(key_file.get_string(group_name, "Comments"))
                : Glib::ustring();
        new_row[model_columns.col_allow_write] =
            key_file.has_key(group_name, "AllowWrite") &&
            key_file.get_string(group_name, "AllowWrite") == "allow";

        auto append_error = [&](const std::string &field,
                                const std::string &value) {
            if (!errors.empty())
                errors += "\n";
            errors += group_name + ": " + field + ": Недопустимое значение '" +
                      value + "'";
        };

        auto get_int_value = [&](const std::string &key,
                                 int default_value) -> int {
            if (!key_file.has_key(group_name, key))
                return default_value;
            std::string value_str = key_file.get_string(group_name, key);
            try {
                size_t pos = 0;
                int val = std::stoi(value_str, &pos);
                if (pos != value_str.size())
                    throw std::invalid_argument("extra chars");
                return val;
            } catch (...) {
                append_error(key, value_str);
                return default_value;
            }
        };

        if (key_file.has_key(group_name, "UDP_port")) {
            std::string port_str = key_file.get_string(group_name, "UDP_port");
            if (port_str.size() > 2) {
                std::string port_hex = port_str.substr(2);
                try {
                    size_t pos = 0;
                    int port_val = std::stoi(port_hex, &pos, 16);
                    if (pos != port_hex.size())
                        throw std::invalid_argument("extra chars");
                    new_row[model_columns.col_port] = port_val;
                } catch (...) {
                    append_error("UDP_port", port_str);
                    new_row[model_columns.col_port] = 25923;
                }
            } else {
                new_row[model_columns.col_port] = 25923;
            }
        } else {
            new_row[model_columns.col_port] = 25923;
        }

        new_row[model_columns.col_timeout] = get_int_value("TimeOut", 500);
        new_row[model_columns.col_tries] = get_int_value("QuanRequest", 3);
        new_row[model_columns.col_timeout0] = get_int_value("T0", 5);
        new_row[model_columns.col_timeout1] = get_int_value("T1", 15);
        new_row[model_columns.col_timeout2] = get_int_value("T2", 10);
        new_row[model_columns.col_timeout3] = get_int_value("T3", 20);
        new_row[model_columns.col_coeff_k] = get_int_value("K", 8);
        new_row[model_columns.col_coeff_w] = get_int_value("W", 12);

        new_row[model_columns.col_is_reserved] =
            key_file.has_key(group_name, "UseReserv") &&
            key_file.get_string(group_name, "UseReserv") == "1";
        new_row[model_columns.col_server_address1] =
            key_file.has_key(group_name, "UDP_addr")
                ? Glib::ustring(key_file.get_string(group_name, "UDP_addr"))
                : "127.0.0.1";
        new_row[model_columns.col_server_address2] =
            key_file.has_key(group_name, "UDP_addr2")
                ? Glib::ustring(key_file.get_string(group_name, "UDP_addr2"))
                : "";
        new_row[model_columns.col_server_address3] =
            key_file.has_key(group_name, "UDP_addr3")
                ? Glib::ustring(key_file.get_string(group_name, "UDP_addr3"))
                : "";
        new_row[model_columns.col_client_address1] =
            key_file.has_key(group_name, "ClntIPAdr1")
                ? Glib::ustring(key_file.get_string(group_name, "ClntIPAdr1"))
                : "127.0.0.1";
        new_row[model_columns.col_client_address2] =
            key_file.has_key(group_name, "ClntIPAdr2")
                ? Glib::ustring(key_file.get_string(group_name, "ClntIPAdr2"))
                : "";
        new_row[model_columns.col_client_address3] =
            key_file.has_key(group_name, "ClntIPAdr3")
                ? Glib::ustring(key_file.get_string(group_name, "ClntIPAdr3"))
                : "";
        new_row[model_columns.col_proxy_address] =
            key_file.has_key(group_name, "UDP_proxy") &&
                    !key_file.get_string(group_name, "UDP_proxy").empty()
                ? Glib::ustring(key_file.get_string(group_name, "UDP_proxy"))
                : Glib::ustring("Нет");
        new_row[model_columns.col_free_read] =
            key_file.has_key(group_name, "FreeRead") &&
            key_file.get_string(group_name, "FreeRead") == "1";
        new_row[model_columns.col_free_write] =
            key_file.has_key(group_name, "FreeWrite") &&
            key_file.get_string(group_name, "FreeWrite") == "1";
    };

    for (const auto &group : groups) {
        if (group.find("Station") == 0) {
            append_station(group, errors);
        }
    }
    return true;
}

int write_config_backup(const std::string &config_path) {
    // Создание резервной копии файла конфигурации с сохранением прав доступа
    // оригинального файла
    std::ifstream src(config_path, std::ios::binary);
    if (!src.is_open()) {
        int err = errno;
        std::cerr << "Не удалось открыть файл конфигурации для резервного "
                     "копирования по пути "
                  << config_path << ": " << strerror(err) << std::endl;
        if (err == ENOENT) {
            return 1;
        } else if (err == EACCES) {
            return 2;
        } else {
            return 3;
        }
    }
    std::filesystem::path source_path(config_path);
    std::filesystem::path backup_path =
        source_path.parent_path() / "Stations.bak";
    std::ofstream dst(backup_path, std::ios::binary | std::ios::trunc);
    if (!dst.is_open()) {
        int err = errno;
        std::cerr
            << "Не удалось открыть файл для записи резервной копии по пути: "
            << backup_path << std::endl;
        if (err == ENOENT) {
            return 1;
        } else if (err == EACCES) {
            return 2;
        } else {
            return 3;
        }
    }
    dst << src.rdbuf();

    // Назначение прав доступа файлу резервной копии (только при создании
    // бекап файла впервые)
    std::error_code ec;
    auto perms = std::filesystem::status(source_path, ec).permissions();
    std::filesystem::permissions(backup_path, perms, ec);
    return 0;
}

int write_config(const std::string &config_path,
                 const MainSettings &main_settings,
                 const Gtk::TreeModel::Row &servers_row,
                 const ModelColumns &model_columns) {
    // Обновление содержимого существующего файла конфигурации
    Glib::KeyFile key_file;
    try {
        key_file.load_from_file(config_path);
    } catch (const Glib::Error &e) {
        std::cerr << "Ошибка чтения файла конфигурации по пути " << config_path
                  << ": " << e.what() << std::endl;
        return e.code();
    }
    // Удаление данных текущего конфиг файла
    auto groups = key_file.get_groups();
    for (const auto &group : groups) {
        key_file.remove_group(group);
    }

    try {
        key_file.set_string("Main", "FileDescription",
                            main_settings.file_description);
        key_file.set_string("Main", "FileVersion",
                            std::to_string(main_settings.file_version));
        key_file.set_integer("Main", "IgnoreLocalSettings", 1);
        key_file.set_integer("Main", "LocalStationID",
                             main_settings.local_station_id);
        key_file.set_integer("Main", "QuanStations",
                             main_settings.stations_amount);
        key_file.set_integer("Main", "QuanClntsStations",
                             main_settings.client_stations_amount);
        // TODO:
        key_file.set_integer("Main", "DebugReg", 0);
        for (const auto &row : servers_row.children()) {
            std::string group_name =
                "Station_" + std::to_string(row[model_columns.col_id]);

            key_file.set_integer(group_name, "OS", 1);
            key_file.set_string(group_name, "OS_str", "Linux");
            key_file.set_integer(group_name, "ID", row[model_columns.col_id]);
            key_file.set_string(group_name, "Name",
                                row[model_columns.col_name]);
            key_file.set_string(group_name, "Comments",
                                row[model_columns.col_comments]);
            key_file.set_string(group_name, "AllowWrite",
                                row[model_columns.col_allow_write] ? "allow"
                                                                   : "no");
            key_file.set_integer(group_name, "T0",
                                 row[model_columns.col_timeout0]);
            key_file.set_integer(group_name, "T1",
                                 row[model_columns.col_timeout1]);
            key_file.set_integer(group_name, "T2",
                                 row[model_columns.col_timeout2]);
            key_file.set_integer(group_name, "T3",
                                 row[model_columns.col_timeout3]);
            std::stringstream ss;
            ss << "0x" << std::hex << row[model_columns.col_port];
            key_file.set_string(group_name, "UDP_port", ss.str());
            key_file.set_integer(group_name, "TimeOut",
                                 row[model_columns.col_timeout]);
            key_file.set_integer(group_name, "QuanRequest",
                                 row[model_columns.col_tries]);
            key_file.set_integer(group_name, "K",
                                 row[model_columns.col_coeff_k]);
            key_file.set_integer(group_name, "W",
                                 row[model_columns.col_coeff_w]);
            key_file.set_string(group_name, "UseReserv",
                                row[model_columns.col_is_reserved] ? "1" : "0");
            key_file.set_string(group_name, "UDP_addr",
                                row[model_columns.col_server_address1]);
            key_file.set_string(group_name, "UDP_addr2",
                                row[model_columns.col_server_address2]);
            key_file.set_string(group_name, "UDP_addr3",
                                row[model_columns.col_server_address3]);
            key_file.set_string(group_name, "ClntIPAdr1",
                                row[model_columns.col_client_address1]);
            key_file.set_string(group_name, "ClntIPAdr2",
                                row[model_columns.col_client_address2]);
            key_file.set_string(group_name, "ClntIPAdr3",
                                row[model_columns.col_client_address3]);
            Glib::ustring proxy_address = row[model_columns.col_proxy_address];
            key_file.set_string(group_name, "UDP_proxy",
                                proxy_address.empty() ? "Нет" : proxy_address);
            key_file.set_integer(group_name, "FreeRead",
                                 row[model_columns.col_free_read] ? 1 : 0);
            key_file.set_integer(group_name, "FreeWrite",
                                 row[model_columns.col_free_write] ? 1 : 0);
        }
        key_file.save_to_file(config_path);
    } catch (const Glib::Error &e) {
        return e.code();
    }

    //   Удаление пустых строк между группами
    Glib::ustring data = key_file.to_data();
    GRegex *regex = g_regex_new("\n\n", static_cast<GRegexCompileFlags>(0),
                                static_cast<GRegexMatchFlags>(0), NULL);
    gchar *fixed_data =
        g_regex_replace_literal(regex, data.c_str(), -1, 0, "\n",
                                static_cast<GRegexMatchFlags>(0), NULL);
    g_regex_unref(regex);
    std::ofstream ofs(config_path);
    ofs << fixed_data;
    ofs.close();
    g_free(fixed_data);
    return 0;
}
