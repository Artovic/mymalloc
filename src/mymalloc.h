#ifndef DEFINE_H
# define DEFINE_H

struct meta
{
  struct meta *prev;
  struct meta *next;
  int free; // 1 = free, used = 0, freed = -1
  size_t size;
};

struct ph
{
  struct ph *prev;
  struct ph *next;
  struct meta *start;
  /*
  ** start size of ph->start->size = ss means page is empty
  */
  size_t ss;
};

#endif
