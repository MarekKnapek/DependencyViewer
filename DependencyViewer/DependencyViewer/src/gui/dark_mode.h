#include "../nogui/my_windows.h"


#if defined __cplusplus
#define externc extern "C"
#else
#define externc
#endif


externc void dark_mode_init(void);
externc void dark_mode_reinit(void);
externc void dark_mode_refresh(void);
externc void dark_mode_window(HWND const hwnd);
externc void dark_mode_ctrl_list_view(HWND const list_view);
externc void dark_mode_ctrl_tree_view(HWND const tree_view);
externc void dark_mode_deinit(void);


#undef externc
