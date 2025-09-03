#include <gtkmm.h>

class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(col_icon);
            add(col_id);
            add(col_name);
            add(col_comments);
            add(col_allow_write);
            add(col_port);
            add(col_timeout);
            add(col_tries);
            add(col_timeout0);
            add(col_timeout1);
            add(col_timeout2);
            add(col_timeout3);
            add(col_coeff_k);
            add(col_coeff_w);
            add(col_is_reserved);
            add(col_server_address1);
            add(col_server_address2);
            add(col_server_address3);
            add(col_client_address1);
            add(col_client_address2);
            add(col_client_address3);
            add(col_proxy_address);
            add(col_free_read);
            add(col_free_write);
        }

        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> col_icon;
        Gtk::TreeModelColumn<int> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_comments;
        Gtk::TreeModelColumn<bool> col_allow_write;
        Gtk::TreeModelColumn<int> col_port;
        Gtk::TreeModelColumn<int> col_timeout;
        Gtk::TreeModelColumn<int> col_tries;
        Gtk::TreeModelColumn<int> col_timeout0;
        Gtk::TreeModelColumn<int> col_timeout1;
        Gtk::TreeModelColumn<int> col_timeout2;
        Gtk::TreeModelColumn<int> col_timeout3;
        Gtk::TreeModelColumn<int> col_coeff_k;
        Gtk::TreeModelColumn<int> col_coeff_w;
        Gtk::TreeModelColumn<bool> col_is_reserved;
        Gtk::TreeModelColumn<Glib::ustring> col_server_address1;
        Gtk::TreeModelColumn<Glib::ustring> col_server_address2;
        Gtk::TreeModelColumn<Glib::ustring> col_server_address3;
        Gtk::TreeModelColumn<Glib::ustring> col_client_address1;
        Gtk::TreeModelColumn<Glib::ustring> col_client_address2;
        Gtk::TreeModelColumn<Glib::ustring> col_client_address3;
        Gtk::TreeModelColumn<Glib::ustring> col_proxy_address;
        Gtk::TreeModelColumn<bool> col_free_read;
        Gtk::TreeModelColumn<bool> col_free_write;
};

struct MainSettings {
    public:
        std::string file_description;
        int file_version;
        bool ingore_local_settings;
        int local_station_id;
        int stations_amount;
        int client_stations_amount;
        bool debug_reg;

        MainSettings() {
            file_description = "SCADA list stations";
            file_version = 1;
            ingore_local_settings = false;
            local_station_id = -1;
            stations_amount = 0;
            client_stations_amount = 0;
            debug_reg = false;
        }
};

int get_missing_id(const Gtk::TreeModel::Row &servers_row,
                   const ModelColumns &model_columns);
void resolve_dns_async(Gtk::Window *window, const std::string &dns,
                       Gtk::Entry &entry);
std::string get_active_ipv4_address();

bool parse_config(const std::string &config_path, MainSettings &main_settings,
                  Gtk::TreeModel::Row &servers_row,
                  Glib::RefPtr<Gtk::TreeStore> &tree_store,
                  const ModelColumns &model_columns, int &newest_station_id,
                  std::string &errors);

int write_config_backup(const std::string &config_path);
int write_config(const std::string &config_path,
                 const MainSettings &main_settings,
                 const Gtk::TreeModel::Row &servers_row,
                 const ModelColumns &model_columns);
