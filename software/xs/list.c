/**************************** XS ********************************************
Copyright (C) 2000-2012  P. Bergman

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
software; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA  02111-1307 USA                             
*****************************************************************************/
#include <stdlib.h>

#include "list.h"

static status allocate_node(list *p_L, generic_ptr data)
{
    list L = (list) malloc(sizeof(node));
  
    if (L == NULL) return Error;
  
    *p_L = L;
  
    DATA(L) = data;
    NEXT(L) = NULL;
    PREV(L) = NULL;
  
    return Ok;
}

static void free_node(list *p_L)
{
    free(*p_L);
    *p_L = NULL;
}

status init_list(list *p_L)
{
    *p_L = NULL;
    return Ok;
}

bool empty_list(list L)
{
    return (L == NULL) ? tRUE : fALSE;
}

status insert(list *p_L, generic_ptr data)
{
    list L;
  
    if (allocate_node(&L, data) == Error)
        return Error;
  
    NEXT(L) = *p_L;
    *p_L = L;
  
    return Ok;
}

status append(list *p_L, generic_ptr data)
{
    list L;
  
    if (allocate_node(&L, data) == Error)
        return Error;
  
    if (empty_list(*p_L))
        *p_L = L;
    else {
        list tmp;
        for (tmp = *p_L; NEXT(tmp) != NULL; tmp = NEXT(tmp));
        NEXT(tmp) = L;
    }
  
    return Ok;
}

status delete_node(list *p_L, list L)
{
    if (empty_list(*p_L))
        return Error;
  
    if (*p_L == L)
        *p_L = NEXT(*p_L);
    else {
        list tmp;
        for (tmp = *p_L; tmp != NULL && NEXT(tmp) != L; tmp = NEXT(tmp));
        if (tmp == NULL)
            return Error;
        else
            NEXT(tmp) = NEXT(L);
    }
    free_node(&L);
    return Ok;
}

status delete(list *p_L, generic_ptr *p_data)
{
    if (empty_list(*p_L))
        return Error;
  
    *p_data = DATA(*p_L);
  
    return delete_node(p_L, *p_L);
}

status traverse(list L, status (*p_func_f)())
{
    if (empty_list(L))
        return Ok;
  
    if ((*p_func_f)(DATA(L)) == Error)
        return Error;
    else
        return traverse(NEXT(L), p_func_f);
}

list list_iterator(list L, list lastreturn)
{
    return (lastreturn == NULL) ? L : NEXT(lastreturn);
}

int count_list(list L)
{
    int n = 0;
    list curr = NULL;
    
    if (empty_list(L)) return n;
    
    while ((curr = list_iterator(L, curr)) != NULL) n++;
    
    return n;
}

void setup_prev(list L)
{
    list curr = NULL, prev = NULL;
    
    while ( (curr = list_iterator(L, prev)) != NULL) {
        PREV(curr) = prev;
        prev = curr;
    }
}

status find_key(list L, generic_ptr key, int (*p_cmp_f)(), list *p_keynode)
{
    list curr = NULL;

    while ( (curr = list_iterator(L, curr)) != NULL) {
        if ((*p_cmp_f)(key, DATA(curr)) == 0) {
            *p_keynode = curr;
            return Ok;
        }
    }

    return Error;
}

void destroy(list *p_L, void (*p_func_f)())
{
    if (empty_list(*p_L) == fALSE) {
        destroy(&NEXT(*p_L), p_func_f);
        if (p_func_f != NULL)
            (*p_func_f)(DATA(*p_L));
        free_node(p_L);
    }
}
