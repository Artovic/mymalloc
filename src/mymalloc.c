#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "mymalloc.h"


//contains first page
static struct ph *h = NULL;

void *ptr_add(void *ptr, size_t i)
{
  char *p = ptr;
  p = p + i;
  return p;
}

void *ptr_sub(void *ptr, size_t i)
{
  char *p = ptr;
  p = p - i;
  return p;
}


//ph = page header
void *init_page(struct ph *prev_ph, size_t size)
{
  size_t lenght = sysconf(_SC_PAGE_SIZE);
  if (size >= lenght + sizeof(struct ph) + sizeof(struct meta))
  {
    lenght = (size / lenght + 1) * lenght;
  }
  void* new_page =  mmap(NULL, lenght, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (new_page == NULL)
  {
    return NULL;
  }

  struct meta *m = ptr_add(new_page, sizeof(struct ph));
  m->free = 1;
  m->size = lenght - sizeof(struct meta) - sizeof(struct ph);
  m->prev = NULL;
  m->next = NULL;

  struct ph *ph = new_page;
  ph->prev = prev_ph;
  ph->next = NULL;
  ph->start = m;
  ph->ss = m->size;

  if (prev_ph)
    prev_ph->next = ph;
  else
    h = ph;
  return ph;
}

void cut_space(struct meta *m, size_t size)
{
  size_t ms = sizeof(struct meta);
  struct meta *new_m = ptr_add(m, size + ms);
  new_m->prev = m;
  new_m->next = m->next;
  if (new_m->next)
    new_m->next->prev = new_m;
  new_m->free = 1;
  new_m->size = m->size - ms - size;
  m->next = new_m;
  m->size = size;
}


__attribute__ (( __visibility__("default")))
void *malloc(size_t size)
{
  if (size == 0)
    return NULL;
  else if (h == NULL)
  {
    init_page(NULL, size);
    if (h == NULL)
      return NULL;
  }

  // Finding free block with enough space
  struct ph *ph = h;
  struct ph *prev = NULL;
  struct meta *m = NULL;
  int found = 0;
  while (ph && !found)
  {
    m = ph->start;
    while (m && (m->free != 1 || m->size < size))
      m = m->next;
    if (m == NULL)
    {
      prev = ph;
      ph = ph->next;
    }
    else
      found = 1;
  }

  if (ph == NULL)
  {
    //printf("New page created\n");
    ph = init_page(prev, size);
    if (ph == NULL)
    {
      //free_all(h);
      return NULL;
    }
    m = ph->start;
  }

  m->free = 0;

  // Cutting block if it's size is > 2 * required size
  if (m->size > 2 * size + sizeof(struct meta))
  {
    cut_space(m, size);
  }

  return m + 1;
}

void check_empty()
{
  struct ph *ph = h;
  struct ph *temp = NULL;
  while (ph)
  {
    temp = ph;
    ph = ph->next;
    if (temp->start->free == 1 && temp->ss == temp->start->size)
    {
      if (temp->prev)
      {
        if (temp->next)
        {
          temp->next->prev = temp->prev;
        }
        temp->prev->next = temp->next;
      }
      else if (temp->next)
      {
        h = temp->next;
        h->prev = NULL;
      }
      if (temp->prev || temp->next)
      {
        //printf("%p munmaped\n", (void *) temp);
        munmap(temp, temp->ss + sizeof(struct meta) + sizeof(struct ph));
      }
    }
  }
}

__attribute__ (( __visibility__("default")))
void free(void *ptr)
{
  if (ptr == NULL)
    return ;
  size_t ms = sizeof(struct meta);
  struct meta *m = ptr_sub(ptr, ms);
  if (m->free != 0)
    return ;
  if (m->prev && m->prev->free == 1)
  {
    m->free = -1;
    if (m->next && m->next->free == 1)
    {
      m->next->free = -1;
      m->prev->size += m->size + m->next->size + 2 * ms;
      if (m->next->next)
      {
        m->prev->next = m->next->next;
        m->next->next->prev = m->prev;
      }
      else
      {
        m->prev->next = NULL;
      }
    }
    else
    {
      m->prev->size += m->size + ms;
      m->prev->next = m->next;
      if (m->next)
        m->next->prev = m->prev;
    }
  }
  else if (m->next && m->next->free == 1)
  {
    m->free = 1;
    m->next->free = -1;
    m->size += m->next->size + ms;
    m->next = m->next->next;
    if (m->next)
      m->next->prev = m;
  }
  else
  {
    m->free = 1;
  }
  check_empty();
}

__attribute__ (( __visibility__("default")))
void *calloc(size_t nmemb, size_t size)
{
  size_t s = nmemb * size;
  char *ptr = malloc(s);
  for (size_t i = 0; i < s; i++)
  {
    ptr[i] = 0;
  }
  return ptr;
}

__attribute__ (( __visibility__("default")))
void *realloc(void *ptr, size_t size)
{
  if (ptr == NULL)
    return malloc(size);
  else if (size == 0)
  {
    free(ptr);
    return NULL;
  }
  size_t ms = sizeof(struct meta);
  struct meta *m = ptr_sub(ptr, ms);
  if (m->size == size)
  {
    return ptr;
  }
  else if (m->size > size + ms)
  {
    cut_space(m, size);
    return ptr;
  }
  // Trying to expand space
  struct meta *mn = m->next;
  if (mn && mn->free == 1 && mn->size >= ms
    && m->size + m->next->size + ms > size)
  {
    cut_space(mn, size - m->size - ms);
    m->next = mn->next;
    mn->next->prev = m;
    m->size = size;
    return ptr;
  }
  // Relocating
  char *new_ptr = malloc(size);
  if (!new_ptr)
    return ptr;
  char *char_ptr = ptr;
  for (size_t i = 0; i < m->size; i++)
    new_ptr[i] = char_ptr[i];
  free(ptr);
  return new_ptr;
}
