#pragma once


struct file_info;


void depth_first_visit(file_info const* const& fi, void(*const& callback_fn)(file_info const* const& fi, void* const& param), void* const& param);
void depth_first_visit(file_info* const& fi, void(*const& callback_fn)(file_info* const& fi, void* const& param), void* const& param);
void children_first_visit(file_info const* const& fi, void(*const& callback_fn)(file_info const* const& fi, void* const& param), void* const& param);
void children_first_visit(file_info* const& fi, void(*const& callback_fn)(file_info* const& fi, void* const& param), void* const& param);
