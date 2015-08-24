/**
File developed from the following tutorial:
http://botbench.com/blog/2013/01/31/tutorial-linked-lists-in-robotc/

Tim Loverin
Evan Michelsen
Sarah Lytle
**/
#ifndef __LINKLIST_H__
#define __LINKLIST_H__

struct tList;
struct tListNode;

void insertNode(tList *list, tListNode *node, tListNode *newNode);
void deleteNode(tList *list, tListNode *obsoleteNode);

/**
Struct for creating a node for a linked list
**/
typedef struct tListNode{
	tListNode *next; //neighboring node
	float value;	//value held by node
}tListNode;

/**
Struct for creating a linked list
**/
typedef struct tList{

	tListNode *head;   // pointer to the node at the head of the list
	tListNode *tail;   // pointer to the node at the top of the list
	int size;          // current size of the list
}tList;

/**
Function to insert a node into a linked list after a given node
**/
void insertNode(tList *list, tListNode *node, tListNode *newNode) {
	if (list->size == 0) {
		// Is the list empty?
		list->head = newNode;
		list->tail = newNode;
		list->size++;
		} else if	(node == NULL){
		// node == NULL, so put the newNode at the start of the list
		newNode->next = list->head;
		list->head = newNode;
		list->size++;
		}	else {
		// Insert the newNode into the list, after the node
		newNode->next = node->next;
		node->next = newNode;

		if(list->tail == node){
			// if the node was the tail, update this pointer to the newNode
			list->tail = newNode;
		}
		list->size++;
	}
}
/**
Function to delete a node from list
**/
void deleteNode(tList *list, tListNode *obsoleteNode) {

	tListNode *nodePtr;

	if (list->size == 1) {
		// We only have one node!
		list->head = NULL;
		list->tail = NULL;
		list->size--;
	} else if (list->head == obsoleteNode) {
		// The node we want to delete is the head of the list
		list->head = obsoleteNode->next;
		list->size--;
	} else {
		// we need to start looking for the obsoleteNode's left neighbour
		nodePtr = list->head;
		while(nodePtr != NULL) {
			// The nodePtr's neighbour is the node we're looking for
			if (nodePtr->next == obsoleteNode) {
				// Check if it is also the tail
				if (obsoleteNode == list->tail) {
					list->tail = nodePtr;
				}
				// Update the node's left neighbour
				nodePtr->next = obsoleteNode->next;
				list->size--;
				return;
			}

			// advance to the next node
			nodePtr = nodePtr->next;
		}
	}
}
#endif
