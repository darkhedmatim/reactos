#ifndef __CMD_H
#define __CMD_H
void netlink_cmd (void);
void ftplink_cmd (void);
void undelete_cmd (void);
void help_cmd (void);
void dirsizes_cmd (void);
int view_file_at_line (char *filename, int plain_view, int internal, 
                       int start_line);
int view_file (char *filename, int normal, int internal);
void view_cmd (WPanel *panel);
void view_simple_cmd (WPanel *panel);
void filtered_view_cmd (WPanel *panel);
void filtered_view_cmd_cpanel (void);
void do_edit (const char *what);
void do_edit_at_line (const char *what, int start_line);
void edit_cmd (WPanel *panel);
void edit_cmd_new (WPanel *panel);
void copy_cmd (void);
void ren_cmd (void);
void reselect_vfs (void);
void copymove_cmd_with_default (int copy, char *thedefault);
void mkdir_cmd (WPanel *panel);
void delete_cmd (void);
void find_cmd (void);
void tree_mode_cmd (void);
void filter_cmd (void);
void set_panel_filter (WPanel *panel);
void set_panel_filter_to (WPanel *p, char *allocated_filter_string);
void reread_cmd (void);
void do_re_sort (WPanel *panel);
void quick_view_cmd (void);
void tree_view_cmd (void);
void ext_cmd (void);
void menu_edit_cmd (void);
void quick_chdir_cmd (void);
void compare_dirs_cmd (void);
void history_cmd (void);
void tree_cmd (void);
void link_cmd (void);
void symlink_cmd (void);
void edit_symlink_cmd (void);
void other_symlink_cmd (void);
void reverse_selection_cmd_panel (WPanel *);
void unselect_cmd_panel (WPanel *);
void select_cmd_panel (WPanel *);
void reverse_selection_cmd (void);
void unselect_cmd (void);
void select_cmd (void);
void swap_cmd (void);
void view_other_cmd (void);
void mkdir_panel_cmd (void);
void edit_panel_cmd (void);
void view_panel_cmd (void);
void quick_cd_cmd (void);
void save_setup_cmd (void);
char *get_random_hint (void);
void source_routing (void);

/* Display mode code */
void info_cmd (void);
void tree_cmd (void);
void listing_cmd (void);
void sort_cmd (void);
void switch_to_listing (int panel_index);
void quick_cmd_no_menu (void);
void info_cmd_no_menu (void);
void quick_view_cmd (void);
void toggle_listing_cmd (void);
void configure_panel_listing (WPanel *p, int view_type, int use_msformat, char *user, char *status);
#endif /* __CMD_H */
