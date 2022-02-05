//  03-solution.h
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-31 bw

#ifndef _03_SOLUTION_H
#define _03_SOLUTION_H

void display_row(bw::BWCRUD & db, const char ** row);
void display_rows(bw::BWCRUD & db);
void newline();
const char * promptline(const char *);

void do_menu(bw::BWCRUD & db);
void do_jump(bw::BWCRUD & db, const char & response);

void do_add(bw::BWCRUD & db);
void do_find(bw::BWCRUD & db);
void do_edit(bw::BWCRUD & db);
void do_list(bw::BWCRUD & db);
void do_delete(bw::BWCRUD & db);
void do_drop_exit(bw::BWCRUD & db);

#endif /* _03_SOLUTION_H */
