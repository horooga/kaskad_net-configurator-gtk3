#include "kaskad_net-configurator-gtk3/utils.hpp"
#include <filesystem>
#include <fstream>
#include <gdk/gdkkeysyms.h>
#include <gdkmm.h>
#include <gtkmm.h>

#define TREE_MIN_WIDTH 400
#define PADDIND_WIDTH 100
#define FORM_MIN_WIDTH 800

class NetworkInteractionConfigurator : public Gtk::Window {
    public:
        NetworkInteractionConfigurator(const std::string &project_path)
            : next_station_id(1),
              config_path(
                  std::filesystem::path(project_path).parent_path().string() +
                  "/Configurator/Stations.ini"),
              unsaved(false) {
            button_make_curr_server.set_label("Сделать текущим сервером");
            label_form_id.set_text("Идентификатор");
            label_form_name.set_text("Название");
            label_form_comments.set_text("Комментарии");
            button_make_default.set_label("По умолчанию");
            button_servers_swap.set_label("Поменять");
            button_clients_swap.set_label("Поменять");
            check_button_autodetection.set_label("Автоопределение");
            check_button_allow_write.set_label(
                "Разрешить управление с этой рабочей станции");
            check_button_is_reserved.set_label("Резервирование");
            check_button_proxy.set_label("Связь через посредника");
            check_button_free_read.set_label(
                "Разрешить чтение значений параметров");
            check_button_free_write.set_label(
                "Разрешить запись значений параметров");
            menuitem_save_label.set_text("Сохранить");
            menuitem_exit_label.set_text("Выход");
            menuitem_new_station_label.set_text("Добавить станцию");
            menuitem_delete_label.set_text("Удалить");
            menuitem_help_label.set_text("Справка");
            button_client_address1_dns.set_label("...");
            button_client_address2_dns.set_label("...");
            button_client_address3_dns.set_label("...");
            button_server_address1_dns.set_label("...");
            button_server_address2_dns.set_label("...");
            button_server_address3_dns.set_label("...");

            set_size_request(TREE_MIN_WIDTH + FORM_MIN_WIDTH + PADDIND_WIDTH,
                             600);
            setup_ui();
            setup_menubuttons();
            setup_buttons();
            setup_signals();
            setup_accel_groups();
            show_all_children();

            setup_data(project_path);
        }

    protected:
        bool is_station() {
            // Чтобы отличать станции от их корня "Серверы"
            if (!curr_iter)
                return false;
            return curr_iter->parent();
        }

        void button_save_enable() {
            button_save_icon.set(pixbuf_save_enabled);
            menuitem_save_icon.set(pixbuf_save_enabled);
            button_save.set_sensitive(true);
            menuitem_save.set_sensitive(true);
            unsaved = true;
        }

        void button_save_disable() {
            button_save_icon.set(pixbuf_save_disabled);
            menuitem_save_icon.set(pixbuf_save_disabled);
            button_save.set_sensitive(false);
            menuitem_save.set_sensitive(false);
            unsaved = false;
        }

        void on_tree_selection_changed() {
            auto sel = tree_view.get_selection();
            auto iter = sel->get_selected();
            if (!iter) {
                vbox_form.hide();
                button_save_disable();
                button_delete.set_sensitive(false);
                return;
            }
            vbox_form.show_all();
            curr_iter = iter;
            if (!is_station()) {
                vbox_form.hide();
                button_save_disable();
                button_delete.set_sensitive(false);
                return;
            }

            vbox_form.show_all();
            button_delete.set_sensitive(true);
            if (check_button_autodetection.get_active()) {
                button_make_curr_server.set_sensitive(false);
            } else {
                button_make_curr_server.set_sensitive(true);
            }
            auto curr_row = *curr_iter;

            entry_id.set_text(std::to_string(curr_row[model_columns.col_id]));
            entry_name.set_text(curr_row[model_columns.col_name]);
            entry_comments.set_text(curr_row[model_columns.col_comments]);
            check_button_allow_write.set_active(
                curr_row[model_columns.col_allow_write]);
            spin_port.set_value(curr_row[model_columns.col_port]);
            spin_timeout.set_value(curr_row[model_columns.col_timeout]);
            spin_tries.set_value(curr_row[model_columns.col_tries]);
            spin_timeout0.set_value(curr_row[model_columns.col_timeout0]);
            spin_timeout1.set_value(curr_row[model_columns.col_timeout1]);
            spin_timeout2.set_value(curr_row[model_columns.col_timeout2]);
            spin_timeout3.set_value(curr_row[model_columns.col_timeout3]);
            spin_coeff_k.set_value(curr_row[model_columns.col_coeff_k]);
            spin_coeff_w.set_value(curr_row[model_columns.col_coeff_w]);
            check_button_is_reserved.set_active(
                curr_row[model_columns.col_is_reserved]);
            entry_server_address1.set_text(
                curr_row[model_columns.col_server_address1]);
            entry_server_address2.set_text(
                curr_row[model_columns.col_server_address2]);
            entry_server_address3.set_text(
                curr_row[model_columns.col_server_address3]);
            entry_client_address1.set_text(
                curr_row[model_columns.col_client_address1]);
            entry_client_address2.set_text(
                curr_row[model_columns.col_client_address2]);
            entry_client_address3.set_text(
                curr_row[model_columns.col_client_address3]);
            check_button_proxy.set_active(
                curr_row[model_columns.col_proxy_address] != "Нет");
            entry_proxy_address.set_text(
                curr_row[model_columns.col_proxy_address]);
            check_button_free_read.set_active(
                curr_row[model_columns.col_free_read]);
            check_button_free_write.set_active(
                curr_row[model_columns.col_free_write]);
        }

        void on_new_station_clicked() {
            int missing_idx = get_missing_id(servers_tree_row, model_columns);
            int new_id;
            if (missing_idx == -1) {
                new_id = next_station_id++;
                main_settings.stations_amount++;
            } else {
                auto sortable =
                    Glib::RefPtr<Gtk::TreeSortable>::cast_dynamic(tree_store);
                sortable->set_sort_column(model_columns.col_id,
                                          Gtk::SortType::SORT_ASCENDING);
                new_id = missing_idx;
            }

            auto new_row = *(tree_store->append(servers_tree_row.children()));
            if (!tree_view.row_expanded(
                    tree_store->get_path(servers_tree_row))) {
                // Раскрытие корня станций при добавлении новой
                tree_view.expand_row(tree_store->get_path(servers_tree_row),
                                     false);
            }
            tree_view.get_selection()->select(new_row);
            curr_iter = new_row;

            (*new_row)[model_columns.col_icon] = pixbuf_station;
            entry_id.set_text(std::to_string(new_id));
            (*new_row)[model_columns.col_id] = new_id;
            entry_name.set_text("Новая станция");
            entry_comments.set_text("");
            check_button_allow_write.set_active(false);
            spin_port.set_value(25923);
            spin_timeout.set_value(500);
            spin_tries.set_value(3);
            spin_timeout0.set_value(5);
            spin_timeout1.set_value(15);
            spin_timeout2.set_value(10);
            spin_timeout3.set_value(20);
            spin_coeff_k.set_value(12);
            spin_coeff_w.set_value(8);
            check_button_is_reserved.set_active(false);
            entry_server_address1.set_text("127.0.0.1");
            entry_server_address2.set_text("");
            entry_server_address3.set_text("");
            entry_client_address1.set_text("127.0.0.1");
            entry_client_address2.set_text("");
            entry_client_address3.set_text("");
            check_button_proxy.set_active(false);
            check_button_free_read.set_active(true);
            check_button_free_write.set_active(false);
        }

        void on_save_clicked() {
            int ret_backup = write_config_backup(config_path);
            int ret_write = write_config(config_path, main_settings,
                                         servers_tree_row, model_columns);
            std::string errors;
            std::filesystem::path p(config_path);
            if (ret_backup == 0 && ret_write == 0) {
                button_save_disable();
                return;
            }
            if (ret_backup == 2) {
                errors += "Недостаточно прав на запись по пути резервной "
                          "копии файла конфигурации: " +
                          p.parent_path().string() + "Stations.bak\n";
            }
            if (ret_write == 2) {
                errors += "Недостаточно прав на запись по пути файла "
                          "конфигурации: " +
                          config_path + "\n";
            }
            // Файл конфигурации не найден
            if (ret_write == 4) {
                std::filesystem::path p(config_path);
                if (!std::filesystem::exists(p.parent_path())) {
                    // Создание директории если не существует и хватает прав
                    std::filesystem::create_directory(p.parent_path());
                }
                std::ofstream f(config_path);
                if (f.is_open()) {
                    f.close();
                    ret_write = write_config(config_path, main_settings,
                                             servers_tree_row, model_columns);
                } else {
                    errors += "Не удалось найти файл конфигурации по "
                              "следующему пути: " +
                              config_path + "\n";
                }
                if (ret_write == 0) {
                    button_save_disable();
                    return;
                }
            }
            if (!errors.empty()) {
                Gtk::MessageDialog dialog(
                    *this,
                    "Не удалось сохранить файл конфигурации \n\n" + errors,
                    false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                dialog.run();
            }
        }

        bool on_exit_clicked() {
            if (unsaved) {
                Gtk::MessageDialog dialog(*this, "Сохранить изменения?", false,
                                          Gtk::MESSAGE_QUESTION,
                                          Gtk::BUTTONS_NONE);
                dialog.add_button("Да", Gtk::RESPONSE_YES);
                dialog.add_button("Нет", Gtk::RESPONSE_NO);

                int result = dialog.run();
                if (result == Gtk::RESPONSE_YES) {
                    on_save_clicked();
                    if (unsaved) {
                        // Отмена выхода если не удалось сохранить файл
                        return true;
                    }
                }
                Gtk::Application::get_default()->quit();
            } else {
                Gtk::MessageDialog dialog(
                    *this, "Вы уверены, что хотите выйти?", false,
                    Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
                dialog.add_button("Да", Gtk::RESPONSE_YES);
                dialog.add_button("Нет", Gtk::RESPONSE_NO);

                int result = dialog.run();
                if (result == Gtk::RESPONSE_YES) {
                    Gtk::Application::get_default()->quit();
                }
            }
            return true;
        }

        void on_delete_clicked() {
            if (!curr_iter)
                return;
            Gtk::MessageDialog dialog(
                *this, "Вы уверены, что хотите удалить эту станцию?", false,
                Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
            dialog.add_button("Да", Gtk::RESPONSE_YES);
            dialog.add_button("Нет", Gtk::RESPONSE_NO);

            int result = dialog.run();
            if (result == Gtk::RESPONSE_YES) {
                tree_store->erase(curr_iter);
                main_settings.stations_amount--;

                auto children = servers_tree_row.children();
                if (children.empty()) {
                    button_save_disable();
                    button_delete.set_sensitive(false);
                    vbox_form.hide();
                }
            }

            int missing_idx = get_missing_id(servers_tree_row, model_columns);
            if (missing_idx = -1) {
                next_station_id--;
            }
            button_save_enable();
        }

    private:
        void setup_ui() {
            auto css = Gtk::CssProvider::create();
            css->load_from_data(R"( 
                * {
                    font-family: Sans;
                    font-size: 14px;
                }
                #read-only {
                    opacity: 0.5;
                }
                #button {
                    border-radius: 0;
                }
                #button-without-border {
                    border-style: none;
                    background-color: transparent;
                }
                #red-label {
                    color: red;
                    font-weight: bold;
                }
            )");
            auto screen = get_screen();
            Gtk::StyleContext::add_provider_for_screen(
                screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

            auto set_margin = [](Gtk::Widget &widget, int margin_horizontal,
                                 int margin_vertical) {
                widget.set_margin_top(margin_vertical);
                widget.set_margin_left(margin_horizontal);
                widget.set_margin_right(margin_horizontal);
                widget.set_margin_bottom(margin_vertical);
            };
            auto add_form_row_to_box = [set_margin](
                                           const Glib::ustring &label_text,
                                           Gtk::Widget &widget, Gtk::Box &box) {
                Gtk::Box *hbox =
                    Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
                Gtk::Label *label = Gtk::manage(new Gtk::Label(label_text));
                hbox->pack_start(*label, Gtk::PACK_SHRINK);
                hbox->pack_start(widget, Gtk::PACK_EXPAND_WIDGET);
                set_margin(widget, 5, 5);
                box.pack_start(*hbox, Gtk::PACK_SHRINK);
            };
            auto add_form_address_row_to_box =
                [set_margin](const Glib::ustring &label_text,
                             Gtk::Widget &widget,
                             Gtk::Button &dns_resolve_button, Gtk::Box &box) {
                    Gtk::Box *hbox = Gtk::manage(
                        new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
                    Gtk::Label *label = Gtk::manage(new Gtk::Label(label_text));
                    hbox->pack_start(*label, Gtk::PACK_SHRINK);
                    hbox->pack_start(widget, Gtk::PACK_EXPAND_WIDGET);
                    hbox->pack_start(dns_resolve_button, Gtk::PACK_SHRINK);
                    set_margin(widget, 5, 5);
                    box.pack_start(*hbox, Gtk::PACK_SHRINK);
                };
            auto create_adjustment = [](double value, double lower,
                                        double upper, double step = 1) {
                return Gtk::Adjustment::create(value, lower, upper, step);
            };

            vbox_main.set_hexpand(true);
            vbox_main.set_vexpand(true);
            scrolled_tree.set_hexpand(true);
            scrolled_tree.set_vexpand(true);
            scrolled_tree.set_min_content_height(600);
            scrolled_tree.set_min_content_width(TREE_MIN_WIDTH);
            tree_view.set_hexpand(true);
            tree_view.set_vexpand(true);
            scrolled_form.set_hexpand(true);
            scrolled_form.set_vexpand(true);
            scrolled_form.set_min_content_height(600);
            scrolled_form.set_min_content_width(FORM_MIN_WIDTH);
            vbox_form.set_hexpand(true);
            vbox_form.set_vexpand(true);
            set_margin(top_bar, 0, 5);
            set_margin(top_bar_separator1, 10, 0);
            set_margin(top_bar_separator2, 10, 0);
            set_margin(combobox_stations_label, 5, 0);
            set_margin(combobox_stations_error_label, 5, 0);
            set_margin(combobox_stations, 5, 0);
            set_margin(frame_general, 10, 1);
            set_margin(vbox_general, 10, 10);
            set_margin(frame_udp_settings, 10, 1);
            set_margin(hbox_udp_settings, 10, 10);
            set_margin(frame_iec_settings, 10, 1);
            set_margin(hbox_iec_settings, 10, 1);
            set_margin(frame_timeouts, 10, 1);
            set_margin(frame_coeffs, 10, 1);
            set_margin(frame_servers, 10, 1);
            set_margin(vbox_main_servers, 10, 10);
            set_margin(frame_clients, 10, 1);
            set_margin(vbox_main_clients, 10, 10);
            set_margin(frame_restrictions, 10, 1);
            set_margin(vbox_restrictions, 10, 10);
            set_margin(button_save, 5, 0);
            set_margin(button_delete, 5, 0);
            frame_timeouts.set_shadow_type(Gtk::SHADOW_NONE);
            frame_coeffs.set_shadow_type(Gtk::SHADOW_NONE);
            button_make_default.set_name("button-without-border");
            button_make_default.set_relief(Gtk::RELIEF_NONE);
            button_servers_swap.set_name("button-without-border");
            button_servers_swap.set_relief(Gtk::RELIEF_NONE);
            button_clients_swap.set_name("button-without-border");
            button_clients_swap.set_relief(Gtk::RELIEF_NONE);
            menubutton_file.set_name("button-without-border");
            menubutton_file.set_relief(Gtk::RELIEF_NONE);
            menubutton_edit.set_name("button-without-border");
            menubutton_edit.set_relief(Gtk::RELIEF_NONE);
            menubutton_help.set_name("button-without-border");
            menubutton_help.set_relief(Gtk::RELIEF_NONE);
            button_server_address1_dns.set_name("button-without-border");
            button_server_address1_dns.set_relief(Gtk::RELIEF_NONE);
            button_server_address2_dns.set_name("button-without-border");
            button_server_address2_dns.set_relief(Gtk::RELIEF_NONE);
            button_server_address3_dns.set_name("button-without-border");
            button_server_address3_dns.set_relief(Gtk::RELIEF_NONE);
            button_client_address1_dns.set_name("button-without-border");
            button_client_address1_dns.set_relief(Gtk::RELIEF_NONE);
            button_client_address2_dns.set_name("button-without-border");
            button_client_address2_dns.set_relief(Gtk::RELIEF_NONE);
            button_client_address3_dns.set_name("button-without-border");
            button_client_address3_dns.set_relief(Gtk::RELIEF_NONE);
            header_bar.set_show_close_button(true);
            header_bar.set_decoration_layout("menu:minimize,maximize,close");
            set_titlebar(header_bar);

            set_title("Настройки сетевого взаимодействия");
            add(vbox_main);
            vbox_main.pack_start(top_bar, Gtk::PACK_SHRINK);
            header_bar.pack_start(menubutton_file);
            header_bar.pack_start(menubutton_edit);
            header_bar.pack_start(menubutton_help);
            top_bar.pack_start(button_new_station, Gtk::PACK_SHRINK);
            button_new_station.set_tooltip_text("Добавить станцию");
            top_bar.pack_start(button_delete, Gtk::PACK_SHRINK);
            button_delete.set_tooltip_text("Удалить станцию");
            top_bar.pack_start(top_bar_separator1, Gtk::PACK_SHRINK);
            top_bar.pack_start(button_save, Gtk::PACK_SHRINK);
            button_save.set_tooltip_text("Сохранить (Ctrl+S)");
            top_bar.pack_start(top_bar_separator2, Gtk::PACK_SHRINK);
            combobox_stations_label.set_text("Текущий сервер");
            top_bar.pack_start(combobox_stations_label, Gtk::PACK_SHRINK);
            top_bar.pack_start(stack_autodetection, Gtk::PACK_SHRINK);
            stack_autodetection.set_transition_type(
                Gtk::STACK_TRANSITION_TYPE_NONE);
            stack_autodetection.add(combobox_stations_error_label);
            stack_autodetection.add(combobox_stations);
            combobox_stations_error_label.set_text("Станция не определена");
            combobox_stations_error_label.set_name("red-label");
            top_bar.pack_start(check_button_autodetection, Gtk::PACK_SHRINK);
            check_button_autodetection.set_tooltip_text(
                "Автоопределение текущей станции");

            vbox_main.pack_start(paned_main, Gtk::PACK_EXPAND_WIDGET);
            paned_main.set_hexpand(true);
            paned_main.add1(scrolled_tree);
            paned_main.child_property_resize(scrolled_tree) = true;
            paned_main.child_property_shrink(scrolled_tree) = true;
            scrolled_tree.set_policy(Gtk::POLICY_AUTOMATIC,
                                     Gtk::POLICY_AUTOMATIC);
            tree_store = Gtk::TreeStore::create(model_columns);
            servers_tree_row = *(tree_store)->append();
            servers_tree_row[model_columns.col_name] = "Серверы";
            scrolled_tree.add(tree_view);
            tree_view.set_model(tree_store);
            tree_view.set_headers_visible(false);
            renderer_icon.set_alignment(0.0, 0.0);
            tree_col_icon_idx.pack_start(renderer_icon, false);
            tree_col_icon_idx.pack_start(renderer_id, false);
            tree_col_icon_idx.add_attribute(renderer_icon.property_pixbuf(),
                                            model_columns.col_icon);
            tree_col_icon_idx.add_attribute(renderer_id.property_text(),
                                            model_columns.col_id);
            tree_view.append_column(tree_col_icon_idx);
            tree_col_name.pack_start(renderer_name, true);
            tree_col_name.add_attribute(renderer_name.property_text(),
                                        model_columns.col_name);
            tree_view.append_column(tree_col_name);
            tree_col_comments.pack_start(renderer_comments, true);
            tree_col_comments.add_attribute(renderer_comments.property_text(),
                                            model_columns.col_comments);
            tree_view.append_column(tree_col_comments);
            // Настройка отображения идентификаторов станций
            tree_col_icon_idx.set_cell_data_func(
                renderer_id,
                [this](Gtk::CellRenderer *renderer,
                       const Gtk::TreeModel::const_iterator &iter) {
                    Gtk::CellRendererText *text_renderer =
                        dynamic_cast<Gtk::CellRendererText *>(renderer);
                    if (!text_renderer)
                        return;
                    if (!iter->parent()) {
                        text_renderer->property_visible() = false;
                        return;
                    }
                    text_renderer->property_visible() = true;
                    int value = (*iter)[model_columns.col_id];
                    std::ostringstream oss;
                    oss << std::setfill('0') << std::setw(3) << value;
                    text_renderer->property_text() = oss.str();
                });

            paned_main.add2(scrolled_form);
            paned_main.child_property_resize(scrolled_form) = true;
            paned_main.child_property_shrink(scrolled_form) = true;
            scrolled_form.add(vbox_form);
            scrolled_form.set_policy(Gtk::POLICY_AUTOMATIC,
                                     Gtk::POLICY_AUTOMATIC);
            vbox_form.pack_start(frame_general, Gtk::PACK_SHRINK);
            frame_general_label.set_markup("<b>Общие</b>");
            frame_general.set_label_widget(frame_general_label);
            frame_general.add(vbox_general);
            vbox_general.pack_start(grid_general, Gtk::PACK_SHRINK);
            grid_general.set_column_spacing(25);
            grid_general.set_row_spacing(5);
            entry_id.set_editable(false);
            entry_id.set_can_focus(false);
            entry_id.set_hexpand(true);
            entry_id.set_name("read-only");
            label_form_id.set_halign(Gtk::ALIGN_START);
            grid_general.attach(label_form_id, 0, 0, 1, 1);
            grid_general.attach(hbox_general_id, 1, 0, 1, 1);
            hbox_general_id.pack_start(entry_id, Gtk::PACK_EXPAND_WIDGET);
            hbox_general_id.pack_start(button_make_curr_server,
                                       Gtk::PACK_SHRINK);
            label_form_name.set_halign(Gtk::ALIGN_START);
            grid_general.attach(label_form_name, 0, 1, 1, 1);
            grid_general.attach(entry_name, 1, 1, 1, 1);
            label_form_comments.set_halign(Gtk::ALIGN_START);
            grid_general.attach(label_form_comments, 0, 2, 1, 1);
            grid_general.attach(entry_comments, 1, 2, 1, 1);
            vbox_general.pack_start(check_button_allow_write, Gtk::PACK_SHRINK);

            frame_udp_settings_label.set_markup("<b>Параметры протокола обмена "
                                                "UDP</b>");
            frame_udp_settings.set_label_widget(frame_udp_settings_label);
            spin_timeout.set_adjustment(
                create_adjustment(500, 0, std::numeric_limits<int>::max(), 1));
            spin_timeout.set_numeric(true);
            spin_tries.set_adjustment(
                create_adjustment(3, 0, std::numeric_limits<int>::max(), 1));
            spin_tries.set_numeric(true);
            Gtk::Label *port_label = Gtk::manage(new Gtk::Label("Порт"));
            hbox_udp_settings.pack_start(*port_label, Gtk::PACK_SHRINK);
            hbox_udp_settings.pack_start(spin_port, Gtk::PACK_EXPAND_WIDGET);
            Gtk::Label *timeout_label =
                Gtk::manage(new Gtk::Label("Тайм-аут, мсек"));
            hbox_udp_settings.pack_start(*timeout_label, Gtk::PACK_SHRINK);
            hbox_udp_settings.pack_start(spin_timeout, Gtk::PACK_EXPAND_WIDGET);
            Gtk::Label *tries_label = Gtk::manage(new Gtk::Label("Попытки"));
            hbox_udp_settings.pack_start(*tries_label, Gtk::PACK_SHRINK);
            hbox_udp_settings.pack_start(spin_tries, Gtk::PACK_EXPAND_WIDGET);
            frame_udp_settings.add(hbox_udp_settings);
            vbox_form.pack_start(frame_udp_settings, Gtk::PACK_SHRINK);

            frame_iec_settings_label.set_markup(
                "<b>Параметры протокола IEC</b>");
            frame_iec_settings.set_label_widget(frame_iec_settings_label);
            frame_iec_settings.add(hbox_iec_settings);
            hbox_iec_settings.pack_start(frame_timeouts, Gtk::PACK_SHRINK);
            frame_timeouts_label.set_text("Тайм-ауты, сек");
            frame_timeouts.set_label_widget(frame_timeouts_label);
            frame_timeouts.add(hbox_timeouts);
            vbox_form.pack_start(frame_iec_settings, Gtk::PACK_SHRINK);
            hbox_timeouts.pack_start(vbox_timeouts1, Gtk::PACK_SHRINK);
            add_form_row_to_box("T0", spin_timeout0, vbox_timeouts1);
            add_form_row_to_box("T1", spin_timeout1, vbox_timeouts1);
            hbox_timeouts.pack_start(vbox_timeouts2, Gtk::PACK_SHRINK);
            add_form_row_to_box("T2", spin_timeout2, vbox_timeouts2);
            add_form_row_to_box("T3", spin_timeout3, vbox_timeouts2);
            spin_timeout0.set_adjustment(
                create_adjustment(5, 0, std::numeric_limits<int>::max(), 1));
            spin_timeout0.set_numeric(true);
            spin_timeout1.set_adjustment(
                create_adjustment(15, 0, std::numeric_limits<int>::max(), 1));
            spin_timeout1.set_numeric(true);
            spin_timeout2.set_adjustment(
                create_adjustment(10, 0, std::numeric_limits<int>::max(), 1));
            spin_timeout2.set_numeric(true);
            spin_timeout3.set_adjustment(
                create_adjustment(10, 0, std::numeric_limits<int>::max(), 1));
            spin_timeout3.set_numeric(true);
            spin_port.set_adjustment(create_adjustment(25923, 1, 65535, 1));
            spin_port.set_numeric(true);

            hbox_iec_settings.pack_start(frame_coeffs, Gtk::PACK_SHRINK);
            frame_coeffs_label.set_text("Коэффиценты");
            frame_coeffs.set_label_widget(frame_coeffs_label);
            frame_coeffs.add(hbox_coeffs);
            hbox_coeffs.pack_start(vbox_coeffs, Gtk::PACK_SHRINK);
            add_form_row_to_box("K", spin_coeff_k, vbox_coeffs);
            add_form_row_to_box("W", spin_coeff_w, vbox_coeffs);
            hbox_coeffs.pack_start(button_make_default, Gtk::PACK_SHRINK);
            spin_coeff_k.set_adjustment(
                create_adjustment(12, 1, std::numeric_limits<int>::max(), 1));
            spin_coeff_k.set_numeric(true);
            spin_coeff_w.set_adjustment(
                create_adjustment(8, 1, std::numeric_limits<int>::max(), 1));
            spin_coeff_w.set_numeric(true);

            vbox_form.pack_start(frame_servers, Gtk::PACK_SHRINK);
            frame_servers_label.set_markup(
                "<b>Свойства TCP/IP для сервера</b>");
            frame_servers.set_label_widget(frame_servers_label);
            frame_servers.add(vbox_main_servers);
            vbox_main_servers.pack_start(check_button_is_reserved,
                                         Gtk::PACK_SHRINK);
            vbox_main_servers.pack_start(hbox_servers, Gtk::PACK_SHRINK);
            hbox_servers.pack_start(vbox_servers, Gtk::PACK_SHRINK);
            add_form_address_row_to_box("Адрес1", entry_server_address1,
                                        button_server_address1_dns,
                                        vbox_servers);
            add_form_address_row_to_box("Адрес2", entry_server_address2,
                                        button_server_address2_dns,
                                        vbox_servers);
            add_form_address_row_to_box("Адрес3", entry_server_address3,
                                        button_server_address3_dns,
                                        vbox_servers);
            button_servers_swap.set_hexpand(false);
            button_servers_swap.set_halign(Gtk::ALIGN_CENTER);
            button_servers_swap.set_valign(Gtk::ALIGN_CENTER);
            hbox_servers.set_homogeneous(false);
            hbox_servers.pack_start(button_servers_swap, Gtk::PACK_SHRINK);

            vbox_form.pack_start(frame_clients, Gtk::PACK_SHRINK);
            frame_clients_label.set_markup(
                "<b>Свойства TCP/IP для клиента</b>");
            frame_clients.set_label_widget(frame_clients_label);
            frame_clients.add(vbox_main_clients);
            vbox_main_clients.pack_start(hbox_clients, Gtk::PACK_SHRINK);
            hbox_clients.pack_start(vbox_clients, Gtk::PACK_SHRINK);
            add_form_address_row_to_box("Адрес1", entry_client_address1,
                                        button_client_address1_dns,
                                        vbox_clients);
            add_form_address_row_to_box("Адрес2", entry_client_address2,
                                        button_client_address2_dns,
                                        vbox_clients);
            add_form_address_row_to_box("Адрес3", entry_client_address3,
                                        button_client_address3_dns,
                                        vbox_clients);
            button_clients_swap.set_hexpand(false);
            button_clients_swap.set_halign(Gtk::ALIGN_CENTER);
            button_clients_swap.set_valign(Gtk::ALIGN_CENTER);
            hbox_clients.set_homogeneous(false);
            hbox_clients.pack_start(button_clients_swap, Gtk::PACK_SHRINK);
            vbox_main_clients.pack_start(check_button_proxy, Gtk::PACK_SHRINK);
            entry_proxy_address.set_hexpand(false);
            entry_proxy_address.set_halign(Gtk::ALIGN_START);
            add_form_row_to_box("Адрес", entry_proxy_address,
                                vbox_main_clients);

            vbox_form.pack_start(frame_restrictions, Gtk::PACK_SHRINK);
            frame_restrictions_label.set_markup("<b>Ограничение доступа</b>");
            frame_restrictions.set_label_widget(frame_restrictions_label);
            frame_restrictions.add(vbox_restrictions);
            label_restrictions_unregistered.set_text(
                "С незарегистрированных рабочих станций");
            label_restrictions_unregistered.set_halign(Gtk::ALIGN_START);
            vbox_restrictions.pack_start(label_restrictions_unregistered);
            vbox_restrictions.pack_start(check_button_free_read,
                                         Gtk::PACK_SHRINK);
            vbox_restrictions.pack_start(check_button_free_write,
                                         Gtk::PACK_SHRINK);
            entry_proxy_address.set_text("Нет");
            entry_proxy_address.set_sensitive(false);
        }

        void setup_menubuttons() {
            pixbuf_save_enabled = Gdk::Pixbuf::create_from_resource(
                "/org/icons/save-enabled.png");
            menubutton_file.set_label("Файл");
            menu_file.append(menuitem_save);
            menuitem_save.add(menuitem_save_box);
            menuitem_save_box.pack_start(menuitem_save_icon, Gtk::PACK_SHRINK);
            menuitem_save_box.pack_start(menuitem_save_label, Gtk::PACK_SHRINK);
            menu_file.append(menuitem_exit);
            menuitem_exit.add(menuitem_exit_box);
            menuitem_exit_box.pack_start(menuitem_exit_label, Gtk::PACK_SHRINK);
            menubutton_file.set_popup(menu_file);
            menu_file.show_all();

            pixbuf_station =
                Gdk::Pixbuf::create_from_resource("/org/icons/server.png");
            pixbuf_new_station =
                Gdk::Pixbuf::create_from_resource("/org/icons/server-add.png");
            menuitem_new_station_icon = Gtk::Image(pixbuf_new_station);
            menubutton_edit.set_label("Правка");
            menu_edit.append(menuitem_new_station);
            menuitem_new_station.add(menuitem_new_station_box);
            menuitem_new_station_box.pack_start(menuitem_new_station_icon,
                                                Gtk::PACK_SHRINK);
            menuitem_new_station_box.pack_start(menuitem_new_station_label,
                                                Gtk::PACK_SHRINK);
            pixbuf_delete =
                Gdk::Pixbuf::create_from_resource("/org/icons/delete.png");
            menuitem_delete_icon = Gtk::Image(pixbuf_delete);
            menu_edit.append(menuitem_delete);
            menuitem_delete.add(menuitem_delete_box);
            menuitem_delete_box.pack_start(menuitem_delete_icon,
                                           Gtk::PACK_SHRINK);
            menuitem_delete_box.pack_start(menuitem_delete_label,
                                           Gtk::PACK_SHRINK);
            menubutton_edit.set_popup(menu_edit);
            menu_edit.show_all();

            pixbuf_help = Gdk::Pixbuf::create_from_resource(
                "/org/icons/question-mark.png");
            menuitem_help_icon = Gtk::Image(pixbuf_help);
            menubutton_help.set_label("Справка");
            menu_help.append(menuitem_help);
            menuitem_help.add(menuitem_help_box);
            menuitem_help_box.pack_start(menuitem_help_icon, Gtk::PACK_SHRINK);
            menuitem_help_box.pack_start(menuitem_help_label, Gtk::PACK_SHRINK);
            menubutton_help.set_popup(menu_help);
            menu_help.show_all();
        }

        void setup_buttons() {
            pixbuf_save_disabled = Gdk::Pixbuf::create_from_resource(
                "/org/icons/save-disabled.png");
            button_save.set_image(button_save_icon);
            button_save.set_always_show_image(true);
            button_save_disable();

            button_delete_icon = Gtk::Image(pixbuf_delete);
            button_delete.set_image(button_delete_icon);
            button_delete.set_always_show_image(true);

            button_new_station_icon = Gtk::Image(pixbuf_new_station);
            button_new_station.set_image(button_new_station_icon);
            button_new_station.set_always_show_image(true);

            button_delete.set_sensitive(false);
        }

        void setup_signals() {
            signal_delete_event().connect([this](GdkEventAny *event) {
                bool ret = on_exit_clicked();
                return ret;
            });
            tree_view.get_selection()->signal_changed().connect(sigc::mem_fun(
                *this,
                &NetworkInteractionConfigurator::on_tree_selection_changed));
            paned_main.property_position().signal_changed().connect([&]() {
                int pos = paned_main.get_position();
                int total_width = paned_main.get_allocation().get_width();

                if (pos < TREE_MIN_WIDTH)
                    paned_main.set_position(TREE_MIN_WIDTH);
                else if (pos > total_width - FORM_MIN_WIDTH)
                    paned_main.set_position(total_width - FORM_MIN_WIDTH);
            });

            menuitem_save.signal_activate().connect(
                [this]() { on_save_clicked(); });
            menuitem_exit.signal_activate().connect(
                [this]() { on_exit_clicked(); });
            menuitem_new_station.signal_activate().connect(
                [this]() { on_new_station_clicked(); });
            menuitem_delete.signal_activate().connect(
                [this]() { on_delete_clicked(); });

            button_new_station.signal_clicked().connect(sigc::mem_fun(
                *this,
                &NetworkInteractionConfigurator::on_new_station_clicked));
            button_save.signal_clicked().connect(sigc::mem_fun(
                *this, &NetworkInteractionConfigurator::on_save_clicked));
            button_delete.signal_clicked().connect(sigc::mem_fun(
                *this, &NetworkInteractionConfigurator::on_delete_clicked));
            button_make_curr_server.signal_clicked().connect([this]() {
                int idx =
                    std::stoi(static_cast<std::string>(entry_id.get_text()));
                Glib::RefPtr<Gtk::TreeModel> model =
                    combobox_stations.get_model();
                if (!model)
                    return;
                auto iter = model->children().begin();
                auto iter_end = model->children().end();

                for (; iter != iter_end; ++iter) {
                    Gtk::TreeModel::Row row = *iter;
                    if (row[model_columns.col_id] == idx) {
                        combobox_stations.set_active(iter);
                        main_settings.local_station_id = idx;
                        break;
                    }
                }
            });
            button_make_default.signal_clicked().connect([this]() {
                spin_port.set_value(25923);
                spin_timeout0.set_value(5);
                spin_timeout1.set_value(15);
                spin_timeout2.set_value(10);
                spin_timeout3.set_value(20);
                spin_coeff_k.set_value(12);
                spin_coeff_w.set_value(8);
            });
            button_server_address1_dns.signal_clicked().connect([this]() {
                resolve_dns_async(this, entry_server_address1.get_text(),
                                  entry_server_address1);
                button_save_enable();
            });
            button_server_address2_dns.signal_clicked().connect([this]() {
                resolve_dns_async(this, entry_server_address2.get_text(),
                                  entry_server_address2);
                button_save_enable();
            });
            button_server_address3_dns.signal_clicked().connect([this]() {
                resolve_dns_async(this, entry_server_address3.get_text(),
                                  entry_server_address3);
                button_save_enable();
            });
            button_client_address1_dns.signal_clicked().connect([this]() {
                resolve_dns_async(this, entry_client_address1.get_text(),
                                  entry_client_address1);
                button_save_enable();
            });
            button_client_address2_dns.signal_clicked().connect([this]() {
                resolve_dns_async(this, entry_client_address2.get_text(),
                                  entry_client_address2);
                button_save_enable();
            });
            button_client_address3_dns.signal_clicked().connect([this]() {
                resolve_dns_async(this, entry_client_address3.get_text(),
                                  entry_client_address3);
                button_save_enable();
            });
            button_servers_swap.signal_clicked().connect([this]() {
                if (entry_server_address3.get_text().empty()) {
                    std::string tmp = entry_server_address1.get_text();
                    entry_server_address1.set_text(
                        entry_server_address2.get_text());
                    entry_server_address2.set_text(tmp);
                } else {
                    std::string tmp = entry_server_address1.get_text();
                    entry_server_address1.set_text(
                        entry_server_address3.get_text());
                    entry_server_address3.set_text(
                        entry_server_address2.get_text());
                    entry_server_address2.set_text(tmp);
                }
            });
            button_clients_swap.signal_clicked().connect([this]() {
                if (entry_client_address3.get_text().empty()) {
                    std::string tmp = entry_client_address1.get_text();
                    entry_client_address1.set_text(
                        entry_client_address2.get_text());
                    entry_client_address2.set_text(tmp);
                } else {
                    std::string tmp = entry_client_address1.get_text();
                    entry_client_address1.set_text(
                        entry_client_address3.get_text());
                    entry_client_address3.set_text(
                        entry_client_address2.get_text());
                    entry_client_address2.set_text(tmp);
                }
            });

            combobox_stations.signal_changed().connect([this]() {
                auto iter = combobox_stations.get_active();
                if (!iter) {
                    return;
                }
                if ((*iter)[model_columns.col_id] ==
                    main_settings.local_station_id) {
                    return;
                }
                main_settings.local_station_id = (*iter)[model_columns.col_id];
                button_save_enable();
            });

            check_button_autodetection.signal_toggled().connect([this]() {
                if (check_button_autodetection.get_active()) {
                    std::string active_ip = get_active_ipv4_address();
                    int i = 0;
                    if (!active_ip.empty()) {
                        bool found = false;
                        Gtk::TreeIter station_iter =
                            servers_tree_row.children().begin();
                        while (station_iter) {
                            if ((*station_iter)[model_columns
                                                    .col_server_address1] ==
                                    active_ip ||
                                (*station_iter)[model_columns
                                                    .col_server_address2] ==
                                    active_ip ||
                                (*station_iter)[model_columns
                                                    .col_server_address3] ==
                                    active_ip) {
                                tree_view.get_selection()->select(station_iter);
                                combobox_stations.set_active(i);
                                found = true;
                                break;
                            }
                            ++station_iter;
                            i++;
                        }
                        if (!found) {
                            stack_autodetection.set_visible_child(
                                combobox_stations_error_label);
                            if (!curr_iter)
                                tree_view.get_selection()->select(
                                    servers_tree_row.children().begin());
                        }
                        main_settings.local_station_id = -1;
                    } else {
                        stack_autodetection.set_visible_child(
                            combobox_stations_error_label);
                    }
                    combobox_stations.set_sensitive(false);
                    button_make_curr_server.set_sensitive(false);
                } else {
                    stack_autodetection.set_visible_child(combobox_stations);
                    main_settings.local_station_id = 1;
                    combobox_stations.set_sensitive(true);
                    button_make_curr_server.set_sensitive(true);
                }
            });
            check_button_allow_write.signal_toggled().connect([this]() {
                if ((*curr_iter)[model_columns.col_allow_write] ==
                    check_button_allow_write.get_active()) {
                    return;
                }
                (*curr_iter)[model_columns.col_allow_write] =
                    check_button_allow_write.get_active();
                button_save_enable();
            });
            check_button_is_reserved.signal_toggled().connect([this]() {
                if ((*curr_iter)[model_columns.col_is_reserved] ==
                    check_button_is_reserved.get_active()) {
                    return;
                }
                (*curr_iter)[model_columns.col_is_reserved] =
                    check_button_is_reserved.get_active();
                button_save_enable();
            });
            check_button_proxy.signal_toggled().connect([this]() {
                entry_proxy_address.set_sensitive(
                    check_button_proxy.get_active());
                if (((*curr_iter)[model_columns.col_proxy_address] == "Нет") ==
                    !check_button_proxy.get_active()) {
                    // Значение чекбокса соответствует значению поля
                    return;
                }
                if (!check_button_proxy.get_active()) {
                    entry_proxy_address.set_text("Нет");
                    (*curr_iter)[model_columns.col_proxy_address] = "Нет";
                } else {
                    entry_proxy_address.set_text("");
                    button_save_enable();
                }
            });
            check_button_free_read.signal_toggled().connect([this]() {
                if ((*curr_iter)[model_columns.col_free_read] ==
                    check_button_free_read.get_active()) {
                    return;
                }
                (*curr_iter)[model_columns.col_free_read] =
                    check_button_free_read.get_active();
                button_save_enable();
            });
            check_button_free_write.signal_toggled().connect([this]() {
                if ((*curr_iter)[model_columns.col_free_write] ==
                    check_button_free_write.get_active()) {
                    return;
                }
                (*curr_iter)[model_columns.col_free_write] =
                    check_button_free_write.get_active();
                button_save_enable();
            });

            entry_name.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_name] ==
                    entry_name.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_name] = entry_name.get_text();
                button_save_enable();
            });
            entry_comments.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_comments] ==
                    entry_comments.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_comments] =
                    entry_comments.get_text();
                button_save_enable();
            });
            entry_server_address1.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_server_address1] ==
                    entry_server_address1.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_server_address1] =
                    entry_server_address1.get_text();
                button_save_enable();
            });
            entry_server_address2.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_server_address2] ==
                    entry_server_address2.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_server_address2] =
                    entry_server_address2.get_text();
                button_save_enable();
            });
            entry_server_address3.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_server_address3] ==
                    entry_server_address3.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_server_address3] =
                    entry_server_address3.get_text();
                button_save_enable();
            });
            entry_client_address1.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_client_address1] ==
                    entry_client_address1.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_client_address1] =
                    entry_client_address1.get_text();
                button_save_enable();
            });
            entry_client_address2.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_client_address2] ==
                    entry_client_address2.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_client_address2] =
                    entry_client_address2.get_text();
                button_save_enable();
            });
            entry_client_address3.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_client_address3] ==
                    entry_client_address3.get_text()) {
                    return;
                }
                (*curr_iter)[model_columns.col_client_address3] =
                    entry_client_address3.get_text();
                button_save_enable();
            });
            entry_proxy_address.signal_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_proxy_address] ==
                    entry_proxy_address.get_text()) {
                    return;
                }
                if (!entry_proxy_address.get_text().empty()) {
                    (*curr_iter)[model_columns.col_proxy_address] =
                        entry_proxy_address.get_text();
                    button_save_enable();
                }
            });

            spin_timeout.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_timeout] ==
                    (int)spin_timeout.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_timeout] =
                    (int)spin_timeout.get_value();
                button_save_enable();
            });
            spin_tries.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_tries] ==
                    (int)spin_tries.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_tries] =
                    (int)spin_tries.get_value();
                button_save_enable();
            });
            spin_port.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_port] ==
                    (int)spin_port.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_port] =
                    (int)spin_port.get_value();
                button_save_enable();
            });
            spin_timeout0.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_timeout0] ==
                    (int)spin_timeout0.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_timeout0] =
                    (int)spin_timeout0.get_value();
                button_save_enable();
            });
            spin_timeout1.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_timeout1] ==
                    (int)spin_timeout1.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_timeout1] =
                    (int)spin_timeout1.get_value();
                button_save_enable();
            });
            spin_timeout2.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_timeout2] ==
                    (int)spin_timeout2.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_timeout2] =
                    (int)spin_timeout2.get_value();
                button_save_enable();
            });
            spin_timeout3.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_timeout3] ==
                    (int)spin_timeout3.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_timeout3] =
                    (int)spin_timeout3.get_value();
                button_save_enable();
            });
            spin_coeff_k.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_coeff_k] ==
                    (int)spin_coeff_k.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_coeff_k] =
                    (int)spin_coeff_k.get_value();
                button_save_enable();
            });
            spin_coeff_w.signal_value_changed().connect([this]() {
                if ((*curr_iter)[model_columns.col_coeff_w] ==
                    (int)spin_coeff_w.get_value()) {
                    return;
                }
                (*curr_iter)[model_columns.col_coeff_w] =
                    (int)spin_coeff_w.get_value();
                button_save_enable();
            });
        }

        void setup_accel_groups() {
            auto accel_group = Gtk::AccelGroup::create();
            add_accel_group(accel_group);
            button_save.add_accelerator("clicked", accel_group, GDK_KEY_s,
                                        Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
        }

        void setup_data(const std::string &project_path) {
            std::filesystem::path p(project_path);
            if (!std::filesystem::exists(p) || p.filename() != "kaskad.kpr") {
                Gtk::MessageDialog dialog(
                    std::string("Не удалось найти файл проекта по пути: ") +
                        project_path,
                    false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                dialog.run();
                Glib::signal_idle().connect_once([]() {
                    auto app = Gtk::Application::get_default();
                    if (app)
                        app->quit();
                });
            }

            std::string errors;
            parse_config(config_path, main_settings, servers_tree_row,
                         tree_store, model_columns, next_station_id, errors);
            if (!errors.empty()) {
                Gtk::MessageDialog dialog(
                    std::string("Некорректные значения в конфигурационнов "
                                "файле\n\n") +
                        errors,
                    false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                dialog.run();
            }
            main_settings.stations_amount = next_station_id - 1;

            if (next_station_id == 1) {
                // Создание первой станции если конфиг пуст
                auto row = *(tree_store->append(servers_tree_row.children()));
                row[model_columns.col_icon] = pixbuf_station;
                row[model_columns.col_id] = next_station_id++;
                row[model_columns.col_name] = "Новая станция";
                row[model_columns.col_comments] = "";
                row[model_columns.col_port] = (int)spin_port.get_value();
                row[model_columns.col_timeout] = (int)spin_timeout.get_value();
                row[model_columns.col_tries] = (int)spin_tries.get_value();
                row[model_columns.col_timeout0] =
                    (int)spin_timeout0.get_value();
                row[model_columns.col_timeout1] =
                    (int)spin_timeout1.get_value();
                row[model_columns.col_timeout2] =
                    (int)spin_timeout2.get_value();
                row[model_columns.col_timeout3] =
                    (int)spin_timeout3.get_value();
                row[model_columns.col_coeff_k] = (int)spin_coeff_k.get_value();
                row[model_columns.col_coeff_w] = (int)spin_coeff_w.get_value();
                row[model_columns.col_is_reserved] =
                    check_button_is_reserved.get_active();
                row[model_columns.col_server_address1] = "127.0.0.1";
                row[model_columns.col_server_address2] = "";
                row[model_columns.col_server_address3] = "";
                row[model_columns.col_client_address1] = "127.0.0.1";
                row[model_columns.col_client_address2] = "";
                row[model_columns.col_client_address3] = "";
                row[model_columns.col_proxy_address] = "Нет";
                row[model_columns.col_free_read] = true;
                row[model_columns.col_free_write] = false;
            } else {
                // Установка иконки серверов
                Gtk::TreeIter station_iter =
                    servers_tree_row.children().begin();
                while (station_iter) {
                    (*station_iter)[model_columns.col_icon] = pixbuf_station;
                    station_iter++;
                }
            }
            servers_tree_row[model_columns.col_icon] = pixbuf_station;

            tree_view.expand_row(tree_store->get_path(servers_tree_row), false);

            Gtk::TreePath servers_path = tree_store->get_path(servers_tree_row);
            auto filter =
                Gtk::TreeModelFilter::create(tree_store, servers_path);
            filter->set_visible_func(
                [](const Gtk::TreeModel::const_iterator &iter) {
                    return true;
                });
            combobox_stations.clear();
            combobox_stations.set_model(filter);
            combobox_stations.pack_start(renderer_name);
            combobox_stations.add_attribute(renderer_name.property_text(),
                                            model_columns.col_name);
            stack_autodetection.set_visible_child(combobox_stations);

            // Выбор текущей станции по LocalStationId
            Gtk::TreeIter station_iter = servers_tree_row.children().begin();
            int i = 0;
            if (main_settings.local_station_id != -1) {
                while (station_iter && i < main_settings.local_station_id - 1) {
                    ++station_iter;
                    i++;
                }
                if (station_iter) {
                    tree_view.get_selection()->select(station_iter);
                    combobox_stations.set_active(i);
                }
            } else {
                check_button_autodetection.set_active(true);
                button_save_disable();
            }
        }

        Gtk::Box vbox_main{Gtk::ORIENTATION_VERTICAL, 10};
        Gtk::HeaderBar header_bar;
        Gtk::Box top_bar{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Separator top_bar_separator1{Gtk::ORIENTATION_VERTICAL};
        Gtk::Separator top_bar_separator2{Gtk::ORIENTATION_VERTICAL};
        Gtk::ComboBoxText combobox_stations;
        Gtk::Label combobox_stations_label;
        Gtk::Paned paned_main{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Box vbox_form{Gtk::ORIENTATION_VERTICAL, 10};
        Gtk::Frame frame_general;
        Gtk::Label frame_general_label;
        Gtk::Box vbox_general{Gtk::ORIENTATION_VERTICAL, 10};
        Gtk::Grid grid_general;
        Gtk::Label label_form_id, label_form_name, label_form_comments;
        Gtk::Box hbox_general_id{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Frame frame_udp_settings;
        Gtk::Label frame_udp_settings_label;
        Gtk::Box hbox_udp_settings{Gtk::ORIENTATION_HORIZONTAL, 10};
        Gtk::Frame frame_iec_settings;
        Gtk::Label frame_iec_settings_label;
        Gtk::Box hbox_iec_settings{Gtk::ORIENTATION_HORIZONTAL, 10};
        Gtk::Frame frame_timeouts;
        Gtk::Label frame_timeouts_label;
        Gtk::Box hbox_timeouts{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Box vbox_timeouts1{Gtk::ORIENTATION_VERTICAL};
        Gtk::Box vbox_timeouts2{Gtk::ORIENTATION_VERTICAL};
        Gtk::Frame frame_coeffs;
        Gtk::Label frame_coeffs_label;
        Gtk::Box hbox_coeffs{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Box vbox_coeffs{Gtk::ORIENTATION_VERTICAL};
        Gtk::Frame frame_servers;
        Gtk::Label frame_servers_label;
        Gtk::Box vbox_main_servers{Gtk::ORIENTATION_VERTICAL, 10};
        Gtk::Box hbox_servers{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Box vbox_servers{Gtk::ORIENTATION_VERTICAL};
        Gtk::Frame frame_clients;
        Gtk::Label frame_clients_label;
        Gtk::Box vbox_main_clients{Gtk::ORIENTATION_VERTICAL, 10};
        Gtk::Box hbox_clients{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Box vbox_clients{Gtk::ORIENTATION_VERTICAL};
        Gtk::Frame frame_restrictions;
        Gtk::Label frame_restrictions_label;
        Gtk::Label label_restrictions_unregistered;
        Gtk::Box vbox_restrictions{Gtk::ORIENTATION_VERTICAL, 10};

        Gtk::MenuButton menubutton_file, menubutton_edit, menubutton_help;
        Gtk::Menu menu_file, menu_edit, menu_help;
        Gtk::MenuItem menuitem_save, menuitem_exit, menuitem_new_station,
            menuitem_delete, menuitem_help;
        Gtk::Label menuitem_save_label, menuitem_exit_label,
            menuitem_new_station_label, menuitem_delete_label,
            menuitem_help_label;
        Gtk::Box menuitem_save_box{Gtk::ORIENTATION_HORIZONTAL, 5},
            menuitem_exit_box{Gtk::ORIENTATION_HORIZONTAL, 5},
            menuitem_new_station_box{Gtk::ORIENTATION_HORIZONTAL, 5},
            menuitem_delete_box{Gtk::ORIENTATION_HORIZONTAL, 5},
            menuitem_help_box{Gtk::ORIENTATION_HORIZONTAL, 5};
        Gtk::Image menuitem_save_icon, menuitem_new_station_icon,
            menuitem_delete_icon, menuitem_help_icon;

        Gtk::Button button_new_station, button_save, button_delete,
            button_make_curr_server, button_make_default,
            button_server_address1_dns, button_server_address2_dns,
            button_server_address3_dns, button_client_address1_dns,
            button_client_address2_dns, button_client_address3_dns,
            button_servers_swap, button_clients_swap;
        Gtk::CheckButton check_button_autodetection, check_button_allow_write,
            check_button_is_reserved, check_button_proxy,
            check_button_free_read, check_button_free_write;
        Gtk::Entry entry_id, entry_name, entry_comments, entry_server_address1,
            entry_server_address2, entry_server_address3, entry_client_address1,
            entry_client_address2, entry_client_address3, entry_proxy_address;
        Gtk::SpinButton spin_timeout, spin_timeout0, spin_timeout1,
            spin_timeout2, spin_timeout3, spin_coeff_k, spin_coeff_w, spin_port,
            spin_tries;
        Gtk::Stack stack_autodetection;
        Gtk::Label combobox_stations_error_label;

        Gtk::Image button_save_icon, button_delete_icon,
            button_new_station_icon;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf_save_enabled, pixbuf_save_disabled,
            pixbuf_station, pixbuf_new_station, pixbuf_delete, pixbuf_help;
        Gtk::CellRendererPixbuf renderer_icon;
        Gtk::CellRendererText renderer_id, renderer_comments, renderer_name;
        Gtk::TreeViewColumn tree_col_icon_idx, tree_col_comments, tree_col_name;

        Gtk::ScrolledWindow scrolled_tree, scrolled_form;
        Gtk::TreeView tree_view;
        Glib::RefPtr<Gtk::TreeStore> tree_store;
        Gtk::TreeModel::Row servers_tree_row;
        Gtk::TreeModel::iterator curr_iter;
        MainSettings main_settings;
        int next_station_id;
        ModelColumns model_columns;
        std::string config_path;
        bool unsaved;
};

int main(int argc, char *argv[]) {
    std::string project_path;
    if (argc > 1) {
        project_path = argv[1];
    } else {
        project_path = "/usr/share/SCADAProject/kaskad.kpr";
    }

    auto app = Gtk::Application::create("com.github.horooga.cpp_gtk3-gui");
    NetworkInteractionConfigurator window(project_path);
    return app->run(window);
}
