#pragma once


struct file_info;


void depth_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data);
void children_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data);
