#include <stdlib.h>
#include <string.h>
#include "kommon.h"

VlistNode* vlnNewNode(void *data) {
    VlistNode* n = (VlistNode *)malloc(sizeof(VlistNode));
    n->prev = n->next = NULL;
    n->data = data;
    return n;
}

Vlist* vlNewList() {
    Vlist *l = (Vlist *)malloc(sizeof(Vlist));
    l->head = l->tail = NULL;
    l->size = 0;
    return l;
}

Vlist* vlPushBack(Vlist* _self, void *data) {
    VlistNode *newNode = vlnNewNode(data);

    if (_self->head == NULL) {
        _self->head = _self->tail = newNode;
    }
    else {
        VlistNode * tail = _self->tail;
        tail->next = newNode;
        newNode->prev = tail;
        _self->tail = newNode;
    }

    _self->size++;

    return _self;
}

// void* vlPopFront(Vlist* _self) {
//     VlistNode *head;
//     void *data;

//     head = _self->head;

//     if (head == NULL) return NULL;

//     data = head->data;

//     _self->head = head->next;
//     if (_self->head) {
//         _self->head->prev = NULL;
//     }
//     else {
//         _self->tail = NULL;
//     }

//     free(head);
//     _self->size--;

//     return data;
// }

void* vlPopBack(Vlist* _self) {
    VlistNode *tail, *prevTail; 
    void * data;

    if (_self->size <= 0) {
        return NULL;
    }

    tail = _self->tail;
    data = tail->data;

    if (_self->size == 1) {
        _self->head = NULL;
        _self->tail = NULL;
        free(tail);
        _self->size = 0;
        return data;
    }

    prevTail = tail->prev;
    prevTail->next = NULL;
    _self->tail = prevTail;
    free(tail);
    _self->size--;

    return data;
}

void vlnDestroy(VlistNode* vln, void (* releaseData)(void *)) {
    if (vln->data != NULL && releaseData) {
        releaseData(vln->data);
        vln->data = NULL;
    }
    free(vln);
}

void vlDestroy(Vlist* _self, void (* releaseData)(void *)) {
    VlistNode *n1, *n2;

    n1 = _self->head;
    while (n1) {
        n2 = n1->next;
        vlnDestroy(n1, releaseData);
        n1 = n2;
    }

    free(_self);
}

const char *_KOCODE_NAME[] = {
    "NUL",
    "PUSH_VAR",
    "PUSH_LOCAL",
    "PUSH_NUM",
    "PUSH_STR",
    "POP",
    "NEG",
    "CONCAT",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "INTDIV",
    "MOD",
    "NOT",
    "AND",
    "OR",
    "EQUAL",
    "NEQ",
    "GT",
    "LT",
    "GTEQ",
    "LTEQ",
    "CALL_BUILT_IN",
    "CALL_USER",
    "ASSIGN_VAR",
    "ASSIGN_LOCAL",
    "GOTO",
    "IFGOTO",
    "RETURN",
    "STOP"
};
