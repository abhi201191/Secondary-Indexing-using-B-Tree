/*
__________________________________________________________________________________
			DBMS ASSIGNMENT 
__________________________________________________________________________________
Name			:	Abhishek Singh
Roll No.		: 	CS1423
Program			:	WAP to implement secondary indexing using B+ Tree.
Submission Deadline	: 	30th March, 2015
__________________________________________________________________________________
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ORDER 7

/* Structure for node of B+ Tree */
typedef struct TNODE
{
	long int ChildIDs[ORDER], keys[ORDER - 1], ID, parentID;
	bool _isLeaf;
	int totalKeys;
	struct TNODE *next;
}TNODE;

/* Structure to store root ID */
typedef struct root
{
	long int ID; 
}ROOT;

long int id = 0;

TNODE *createNode();
TNODE *createTree(long int key, long int value);
void insertIntoInternalNode(TNODE *parentNode, int index, long int newKey, TNODE *rightChild, FILE *BTFile);
void splitInternalNodeAndInsert(TNODE *parentNode, int index, long int newKey, TNODE *rightChild, FILE *BTFile);
void insertIntoParent(TNODE *leftChild, long int newKey, TNODE *rightChild, FILE *BTFile);
void insertIntoLeaf(TNODE *leafNode, long int key, long int value, FILE *BTFile);
void splitLeafAndInsert(TNODE *leafNode, long int key, long int value, FILE *BTFile);
void insert(long int key, long int value, FILE *BTFile);
TNODE *findNode(FILE *BTFile, long int key);
void findRecord(long int key, FILE *BTFile, FILE *recordFile);
void enqueue(TNODE *childNode);
TNODE * dequeue();
int pathToRoot(TNODE *root, TNODE *node, FILE *BTFile);
void printTree(TNODE *root, FILE *BTFile);
void rangeSearch(long int key1, long int key2, FILE *BTFile, FILE *recordFile);
 
TNODE *queue = NULL;

/* Function to enqueue node in queue */
void enqueue(TNODE *childNode) 
{
	TNODE *c;
	TNODE *node = (TNODE *) malloc(sizeof(TNODE));
	*node = *childNode;
	
	/* If queue is empty, add node and make queue pointer pointing to it */
	if (queue == NULL) 
	{
		queue = node;
		queue->next = NULL;
	}
	
	/* If queue is not empty */
	else 
	{
		c = queue;
		
		/* Traverse queue to the last node */ 
		while(c->next != NULL) 
			c = c->next;
			
		/* Add new node next to the last element in queue */	
		c->next = node;
		node->next = NULL;
	}
}

/* Function to dequeue element from queue */
TNODE * dequeue() 
{
	TNODE *node = queue;
	queue = queue->next;
	node->next = NULL;
	return node;
}

/* Function to return length of the path from some node to root node */
int pathToRoot(TNODE *root, TNODE *node, FILE *BTFile)
{
	int length = 0;
	TNODE *childNode = (TNODE *) malloc(sizeof(TNODE));
	childNode = node;
	
	/* loop until childNode parentID is not same as ID of root node */
	while(childNode->parentID != root->ID)
	{
		/* Read parent node of childNode from file */
		fseek(BTFile, sizeof(ROOT) + (childNode->parentID - 1) * sizeof(TNODE), SEEK_SET);
		fread(childNode, sizeof(TNODE), 1, BTFile);
		length++;
	}
	
	/* Free memory allocated to childNode */
	free(childNode);
	printf("%d length", length);
	return length;
}

/* Function to print tree in level order */
void printTree(TNODE *root, FILE *BTFile)
{
	int i, level, newLevel = 0;
	
	/* Enqueue root node to queue */
	enqueue(root);
	TNODE *tempNode, *parentNode, *childNode;
	
	/* Allocate memory to parentNode and childNode */
	parentNode = (TNODE *) malloc(sizeof(TNODE));
	if(parentNode == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	childNode = (TNODE *) malloc(sizeof(TNODE));
	if(childNode == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	
	/* Loop untill queue is not empty */
	while(queue != NULL)
	{
		/* Dequeue the element from queue */
		tempNode = dequeue();
		
		printf(" |");
				
		/* Print all the keys stored in the tempNode */
		for(i = 0; i < tempNode->totalKeys; i++)
		{
			printf("%ld ", tempNode->keys[i]);
		}
		
		/* If tempNode is not leaf node, then enqueue its all children */   
		if(tempNode->_isLeaf != true)
		{
			for(i = 0; i <= tempNode->totalKeys; i++)
			{
				fseek(BTFile, sizeof(ROOT) + (tempNode->ChildIDs[i] - 1) * sizeof(TNODE), SEEK_SET);
				fread(childNode, sizeof(TNODE), 1, BTFile);
				enqueue(childNode);
			}
		}
		printf("| ");
	}
	
	/* Free memory allocated to parentNode and childNode */	
	free(parentNode);
	free(childNode);
}

/* Function to find the record present between to keys */
void rangeSearch(long int key1, long int key2, FILE *BTFile, FILE *recordFile)
{
	int j, i = 0;
	
	/* Allocate memory to node */
	TNODE *node = (TNODE *) malloc(sizeof(TNODE));
	if(node == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	
	/* Function call to find the node where key is present */
	node = findNode(BTFile, key1);
	
	long int pointer;
	char ch;
	
	/* Loop untill record corresponding to key2 is not traversed */
	do
	{
		for(i = 0; i < node->totalKeys; i++)
		{
			/* Break the loop if one of the key in node is in between key1 and key2 */
			if(node->keys[i] >= key1 && node->keys[i] <= key2)
				break;
		}
	
		/* Prints all the record present in node untill node->keys[i] is greater than key2 */
		do
		{
			/* Read the corresponding entry in ChildIDs[i] field and seek to that position in recordFile and print the record */ 
			pointer = node->ChildIDs[i];
			printf("\nRecord corresponding to key %ld:\n--", node->keys[i]);
			fseek(recordFile, pointer, SEEK_SET);
			do
			{	
				ch = fgetc(recordFile);
				printf("%c", ch);
			}while(ch != '\n');
			
			printf("\n");
			i++;
			
		}while(i < node->totalKeys && node->keys[i] <= key2);
		
		/* Read the adjacent leaf node if node's last pointer is pointing to other leaf */
		if(i == node->totalKeys && node->ChildIDs[ORDER - 1] != -1)
		{
			fseek(BTFile, sizeof(ROOT) + (node->ChildIDs[ORDER - 1] - 1) * sizeof(TNODE), SEEK_SET);
			fread(node, sizeof(TNODE), 1, BTFile);
		}
		else
			break;
			
	}while(node->keys[0] <= key2);
	
	/* Free memory allocated to node */
	free(node);
}

/* Function to find record corresponding to key */
void findRecord(long int key, FILE *BTFile, FILE *recordFile)
{
	int i = 0;
	
	/* Function call to find the node in which key is present */ 
	TNODE *node = findNode(BTFile, key);
	long int pointer;
	char ch;
	
	if(node == NULL)
	{
		printf("\nRecord is not present\n");
	}
	
	else
	{
		/* Loop till node->keys[i] value is equal to key */
		for(i = 0; i < node->totalKeys; i++)
		{
			if(node->keys[i] == key)
				break;
		}
		
		if(i == node->totalKeys)
			printf("\nRecord is not present\n");
		else
		{
			/* Read the corresponding entry in ChildIDs[i] field and seek to that position in recordFile and print the record */ 	
			pointer = node->ChildIDs[i];
			printf("\nRecord corresponding to key %ld:\n--", node->keys[i]);
			fseek(recordFile, pointer, SEEK_SET);
			do
			{	
				ch = fgetc(recordFile);
				printf("%c", ch);
			}while(ch != '\n');
		}	
	}
}

/* Function to create node of TNODE type */
TNODE *createNode()
{
	TNODE *node;
	
	/* Allocate memory to node */
	node = (TNODE *) malloc(sizeof(TNODE));
	if(node == NULL)
	{
		printf("\n..No enough memory..\nError in creating node.\n");
		exit(0);	
	}
	
	node->_isLeaf = false;
	node->parentID = -1;
	node->totalKeys = 0;
	node->ID = ++id;
	return node;
}

/* Function to create tree */
TNODE *createTree(long int key, long int value)
{
	/* Function call to create node and stores the address of that node to root pointer */ 
	TNODE *root = createNode();
	
	/* Make _isLeaf parameter to true because it is first node which itself is leaf node */
	root->_isLeaf = true;
	
	/* Stores key and value */
	root->keys[0] = key;
	root->ChildIDs[0] = value;
	root->totalKeys++;
	return root;
}

/* Function call to insert key into internal node if it is not full */ 
void insertIntoInternalNode(TNODE *parentNode, int index, long int newKey, TNODE *rightChild, FILE *BTFile)
{
	int i;
	
	/* Shift all the entries of keys and ChildIDs by one to create space for newKey to insert into its appropriate position */
	for(i = parentNode->totalKeys; i > index; i--)
	{
		parentNode->ChildIDs[i + 1] = parentNode->ChildIDs[i];
		parentNode->keys[i] = parentNode->keys[i - 1]; 
	}

	/* Store newKey and ChildIDs */
	parentNode->keys[index] = newKey;
	parentNode->ChildIDs[index + 1] = rightChild->ID;
	parentNode->totalKeys++;
	
	/* Write back the modified node into file */
	fseek(BTFile, sizeof(ROOT) + (parentNode->ID - 1) * sizeof(TNODE), SEEK_SET);
	fwrite(parentNode, sizeof(TNODE), 1, BTFile);
	
	/* Free memory allocated to parentNode and rightChild */
	free(parentNode);
	free(rightChild);
}

/* Function to insert key in internal node if it is full. So first split the node and then insert */
void splitInternalNodeAndInsert(TNODE *parentNode, int index, long int newKey, TNODE *rightChild, FILE *BTFile)
{
	/* Allocation memory to newInternalNode and childNode */
	TNODE *newInternalNode = (TNODE *) malloc(sizeof(TNODE));
	if(newInternalNode == NULL)
	{
		printf("\n..No enough memory..\nError in creating node.\n");
		exit(0);	
	}
	
	TNODE *childNode = (TNODE *) malloc(sizeof(TNODE));
	if(childNode == NULL)
	{
		printf("\n..No enough memory..\nError in creating node.\n");
		exit(0);	
	}
	
	long int *tempKeys, *tempChildIDs, childID;
	int i, j, splitPosition;
	
	/* Create some temporary memory for storing keys and child id's */	
	tempKeys = (long int *) calloc(ORDER, sizeof(long int));
	if(tempKeys == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}	
	
	tempChildIDs = (long int *) calloc(ORDER + 1, sizeof(long int));
	if(tempChildIDs == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}

	/* Stores all the ChildIDs entries till index value in tempChildIDs */	
	j = 0;
	for(i = 0; i <= index; i++)
	{
		tempChildIDs[j] = parentNode->ChildIDs[i];
		j++;
	}
	
	/* Insert new ChildID in tempChildIDs */ 
	tempChildIDs[index + 1] = rightChild->ID;
	j++; 
	
	/* Stores remaining childIDs entries */
	for(i = index + 1; i < parentNode->totalKeys + 1; i++)
	{
		tempChildIDs[j] = parentNode->ChildIDs[i];
		j++;
	}
	
	/* Stores all the key values till index value in tempChildIDs */	
	j = 0;
	for(i = 0; i < index; i++)
	{
		tempKeys[j] = parentNode->keys[i];
		j++;
	}
	
	/* Insert newKey in tempChildIDs */
	tempKeys[index] = newKey;
	j++;
	
	/* Stores remaining key values */
	for(i = index; i < parentNode->totalKeys; i++)
	{
		tempKeys[j] = parentNode->keys[i];
		j++;
	}
	
	/* Find the spliting position */
	if(ORDER % 2 == 0)
		splitPosition = ORDER / 2;
	else
		splitPosition = (ORDER / 2) + 1;
	
	/* Function call to create new node */ 
	newInternalNode = createNode();
	parentNode->totalKeys = 0;
	
	/* Stores back all the values of key and childIDs to parentNode till spliting position */ 
	for(i = 0; i < splitPosition; i++)
	{
		parentNode->ChildIDs[i] = tempChildIDs[i];
		parentNode->keys[i] = tempKeys[i];
		parentNode->totalKeys++;
	}
	
	parentNode->ChildIDs[i] = tempChildIDs[i];
	newKey = tempKeys[i];
	
	/* Stores back all the values of key and childIDs to newInternalNode after the split position */ 
	j = 0;
	for(i = splitPosition + 1; i < ORDER; i++)
	{
		newInternalNode->ChildIDs[j] = tempChildIDs[i];
		newInternalNode->keys[j] = tempKeys[i];
		newInternalNode->totalKeys++;
		j++; 
	}
	
	newInternalNode->ChildIDs[j] = tempChildIDs[i];
	
	/* Change the parentID of newInternalNode to same as of parentNode */
	newInternalNode->parentID = parentNode->parentID;
	
	/* Write back both newInternalNode and parentNode to file */
	fseek(BTFile, sizeof(ROOT) + (parentNode->ID - 1) * sizeof(TNODE), SEEK_SET);
	fwrite(parentNode, sizeof(TNODE), 1, BTFile);
	
	fseek(BTFile, sizeof(ROOT) + (newInternalNode->ID - 1) * sizeof(TNODE), SEEK_SET);
	fwrite(newInternalNode, sizeof(TNODE), 1, BTFile);

	/* Read all the children of newInternalNode and change its parentID to ID of newInternalNode and then write back*/	
	for(i = 0; i <= newInternalNode->totalKeys; i++)
	{
		childID = newInternalNode->ChildIDs[i];
		fseek(BTFile, sizeof(ROOT) + (childID - 1) * sizeof(TNODE), SEEK_SET);
		fread(childNode, sizeof(TNODE), 1, BTFile);
		childNode->parentID = newInternalNode->ID;
		fseek(BTFile, sizeof(ROOT) + (childID - 1) * sizeof(TNODE), SEEK_SET);
		fwrite(childNode, sizeof(TNODE), 1, BTFile);
	}
	
	/* Free memory allocated to childNode, tempKeys and tempChildIDs */	
	free(tempKeys);
	free(tempChildIDs);
	free(childNode);
	
	/* Function call to insert newKey to their parent node */
	insertIntoParent(parentNode, newKey, newInternalNode, BTFile);
}

/* Function to insert key into parent */
void insertIntoParent(TNODE *leftChild, long int newKey, TNODE *rightChild, FILE *BTFile)
{
	int index = 0;
	long int parentID;
	
	/* Allocate memory to parentNode */
	TNODE *parentNode = (TNODE *) malloc(sizeof(TNODE));
	if(parentNode == NULL)
	{
		printf("\n..No enough memory..\nError in creating node.\n");
		exit(0);	
	}
	
	ROOT *r;
	
	/* Gets the parentID of leftChild */
	parentID = leftChild->parentID;
	
	/* If leftChild has no parent then create new parent node which will be the new root node */
	if(parentID == -1)
	{
		/* Function call to create new node */
		TNODE *newRoot = createNode();
		newRoot->keys[0] = newKey;
		
		/* Stores ID of left and right child to its ChildIDs array */
		newRoot->ChildIDs[0] = leftChild->ID;
		newRoot->ChildIDs[1] = rightChild->ID;
		newRoot->totalKeys++;
		newRoot->parentID = -1;
		
		/* Make parentID of leftChild and rightChild same as of ID of newRoot */
		leftChild->parentID = newRoot->ID;
		rightChild->parentID = newRoot->ID;
		
		/* Write the newRoot into file */
		fseek(BTFile, sizeof(ROOT) + (newRoot->ID - 1) * sizeof(TNODE), SEEK_SET);
		fwrite(newRoot, sizeof(TNODE), 1, BTFile);
		rewind(BTFile);
		
		/* Read structure from file which stores the id of root node */
		fread(r, sizeof(ROOT), 1, BTFile);
		
		/* Change the ID value of r to ID of newRoot */
		r->ID = newRoot->ID;
		rewind(BTFile);
		
		/* Write back the structure to file */
		fwrite(r, sizeof(ROOT), 1, BTFile);
		
		
		/* Write back both leftChild and rightChild to file */
		fseek(BTFile, sizeof(ROOT) + (leftChild->ID - 1) * sizeof(TNODE), SEEK_SET);
		fwrite(leftChild, sizeof(TNODE), 1, BTFile);
	
		fseek(BTFile, sizeof(ROOT) + (rightChild->ID - 1) * sizeof(TNODE), SEEK_SET);
		fwrite(rightChild, sizeof(TNODE), 1, BTFile);
	}

	/* If leftChild has parent */
	else
	{
		/* Read the parent of leftChild in parentNode from file */
		fseek(BTFile, sizeof(ROOT) + (parentID - 1) * sizeof(TNODE), SEEK_SET);
		fread(parentNode, sizeof(TNODE), 1, BTFile);
		
		/* Find the position in parentNode where leftChild is linked */
		while(index <= parentNode->totalKeys && parentNode->ChildIDs[index] != leftChild->ID)
			index++;
	
		/* If parentNode is not full then call insertIntoInternalNode function to insert newKey value */
		if(parentNode->totalKeys < ORDER - 1)
			insertIntoInternalNode(parentNode, index, newKey, rightChild, BTFile);
			
		/* If parentNode is full then call splitInternalNodeAndInsert function to insert newKey value */
		else
			splitInternalNodeAndInsert(parentNode, index, newKey, rightChild, BTFile);
	}
}


/* Function to insert key into leaf node if it is not full*/ 
void insertIntoLeaf(TNODE *leafNode, long int key, long int value, FILE *BTFile)
{
	int i, position;
	position = 0;
	
	/* Find the appropriate position where key will be stored */
	while(leafNode->keys[position] < key && position < leafNode->totalKeys)
		position++;
	
	/* Shift all the entries of keys and ChildIDs by one to create space for newKey to insert into its appropriate position */
	for(i = leafNode->totalKeys; i > position; i--)
	{
		leafNode->keys[i] = leafNode->keys[i - 1];
		leafNode->ChildIDs[i] = leafNode->ChildIDs[i - 1];
	} 
	
	/* Store newKey and ChildIDs */
	leafNode->keys[position] = key;
	leafNode->ChildIDs[position] = value;
	leafNode->totalKeys++;
	
	/* Write back the modified leaf node into file */
	fseek(BTFile, sizeof(ROOT) + (leafNode->ID - 1) * sizeof(TNODE), SEEK_SET);
	fwrite(leafNode, sizeof(TNODE), 1, BTFile);
	
	/* Free memory allocated to leafNode */
	free(leafNode);	
} 

/* Function to insert key into leaf node if it is full. So first split the node and then insert */ 
void splitLeafAndInsert(TNODE *leafNode, long int key, long int value, FILE *BTFile)
{
	TNODE *newLeafNode;
	long int *tempKeys, *tempChildIDs, newKey;
	int position, i, j, splitPosition; 
	
	/* Function call to create new node */
	newLeafNode = createNode();
	newLeafNode->_isLeaf = true;
	
	/* Create some temporary memory for storing keys and child id's */	 
	tempKeys = (long int *) calloc(ORDER, sizeof(long int));
	if(tempKeys == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}	
	
	tempChildIDs = (long int *) calloc(ORDER, sizeof(long int));
	if(tempChildIDs == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}	
	
	/* Find the appropriate positoin where new key to be inserted */
	position = j = 0;
	while(position < leafNode->totalKeys && leafNode->keys[position] < key)
		position++;
	
	/* Stores all keys and ChildIDs entries till position value in tempChildIDs and tempKeys*/	
	for(i = 0; i < position; i++)
	{
		tempKeys[j] = leafNode->keys[i];
		tempChildIDs[j] = leafNode->ChildIDs[i];
		j++;
	}
	
	/* Store key and ChildID */
	tempKeys[position] = key;
	tempChildIDs[position] = value;
	j++;
	
	/* Stores all remaining keys and ChildIDs entries tempChildIDs and tempKeys */	
	for(i = position; i < leafNode->totalKeys; i++)
	{
		tempKeys[j] = leafNode->keys[i];
		tempChildIDs[j] = leafNode->ChildIDs[i];
		j++;
	
	}
	
	leafNode->totalKeys = 0;
	
	/* Compute the split position */
	if((ORDER - 1) % 2 == 0)
		splitPosition = (ORDER - 1) / 2;
	else
		splitPosition = ((ORDER - 1) / 2) + 1;
	
	/* Stores back all the values of key and childIDs to leafNode till spliting position */  
	for(i = 0; i < splitPosition; i++)
	{
		leafNode->keys[i] = tempKeys[i];
		leafNode->ChildIDs[i] = tempChildIDs[i];
		leafNode->totalKeys++;
	} 

	/* Stores back all the values of key and childIDs to newLeafNode after the split position */ 
	j = 0;
	for(i = splitPosition; i < ORDER; i++)
	{
		newLeafNode->keys[j] = tempKeys[i];
		newLeafNode->ChildIDs[j] = tempChildIDs[i];
		newLeafNode->totalKeys++;
		j++;
	} 
	
	for(i = leafNode->totalKeys; i < ORDER - 1; i++)
		leafNode->ChildIDs[i] = -1;
	
	for(i = newLeafNode->totalKeys; i < ORDER - 1; i++)
		newLeafNode->ChildIDs[i] = -1;

	/* Replace the IDs of next leaf node in both newLeafNode and leafNode */		
	newLeafNode->ChildIDs[ORDER - 1] = leafNode->ChildIDs[ORDER - 1];
	leafNode->ChildIDs[ORDER - 1] = newLeafNode->ID;
	
	/* Make parentID of newLeafNode same as of parentID of leafNode */
	newLeafNode->parentID = leafNode->parentID;
	newKey = leafNode->keys[leafNode->totalKeys - 1];
	
	/* Write back both leafNode and newLeafNode in file */
	fseek(BTFile, sizeof(ROOT) + (leafNode->ID - 1) * sizeof(TNODE), SEEK_SET);
	fwrite(leafNode, sizeof(TNODE), 1, BTFile);
	
	fseek(BTFile, sizeof(ROOT) + (newLeafNode->ID - 1) * sizeof(TNODE), SEEK_SET);
	fwrite(newLeafNode, sizeof(TNODE), 1, BTFile);

	/* Free memory allocated to tempKeys and tempChildIDs */
	free(tempKeys);
	free(tempChildIDs);
	
	/* Function call to insert newKey in parent node */
	insertIntoParent(leafNode, newKey, newLeafNode, BTFile);
}

/* Function to insert key in tree */
void insert(long int key, long int value, FILE *BTFile)
{
	TNODE *leafNode;
	ROOT *r = (ROOT *) malloc(sizeof(ROOT));
	rewind(BTFile);
	
	/* If file is empty then call createTree function to create new tree */
	if(fgetc(BTFile) == EOF)
	{
		leafNode = createTree(key, value);
		
		/* Write the structure in starting of file which contains the ID of root node */
		r->ID = leafNode->ID;
		rewind(BTFile);
		fwrite(r, sizeof(ROOT), 1, BTFile);
		
		/* Write the leafNode */
		fwrite(leafNode, sizeof(TNODE), 1, BTFile);
		
		/* Free memory allocated to r */
		free(r);
	}
	
	/* If file is not empty i.e. some tree is already present */
	else
	{
		rewind(BTFile);
		
		/* Function call to find the node where key to be inserted */
		leafNode = findNode(BTFile, key);

		/* If leafNode is not full then call insertIntoLeaf function */
		if(leafNode->totalKeys < ORDER - 1)
		{
			insertIntoLeaf(leafNode, key, value, BTFile);
		}	

		/* If leafNode is full then call splitLeafAndInsert function */
		else
		{
			splitLeafAndInsert(leafNode, key, value, BTFile);
		}
	}
}

/* Function to find node where key will be present */
TNODE *findNode(FILE *BTFile, long int key)
{
	int i, ID;
	
	/* Allocate memory to r and temp */
	ROOT *r = (ROOT*)malloc(sizeof(ROOT));
	if(r == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	
	TNODE *temp = (TNODE *) malloc(sizeof(TNODE));
	if(temp == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	
	rewind(BTFile);
	
	/* If file is empty then no tree is present */
	if(fgetc(BTFile) == EOF)
	{
		printf("\nTree is Empty\n");
		exit(0);
	}
	
	/* Read the structure which contains the ID of root node */
	rewind(BTFile);
	fread(r, sizeof(ROOT), 1, BTFile);
	ID = r->ID;
	
	/* Seek to root position and read root node in temp */ 
	fseek(BTFile, sizeof(ROOT) + (ID - 1) * sizeof(TNODE), SEEK_SET);
	fread(temp, sizeof(TNODE), 1, BTFile);
	
	/* Loop till temp node is not leaf */ 
	while(temp->_isLeaf != true)
	{
		i = 0;
		while(i < temp->totalKeys)
		{
			if(key > temp->keys[i])
				i++;
			else
				break;
		}
		
		/* Read the appropriate childNode where key is present */
		ID = temp->ChildIDs[i];
		fseek(BTFile, sizeof(ROOT) + (ID - 1) * sizeof(TNODE), SEEK_SET);
		fread(temp, sizeof(TNODE), 1, BTFile);
	}
	
	/* Free memory allocated to r */
	free(r);
	
	return temp;
}

/* DRIVER function */
int main()
{
	FILE *recordFile, *BTFile;
	long int key, key2;
	long int pointer;
	int choice;
	char ch;
	
	/* Allocate memory to root and r */
	TNODE *root = (TNODE *) malloc(sizeof(TNODE));
	if(root == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	
	ROOT *r = (ROOT *) malloc(sizeof(ROOT));
	if(r == NULL)
	{
		printf("\nNot enough memory\n");
		exit(0);
	}
	
	/* Open recordFile in read mode */
	recordFile = fopen("Data.txt", "r");
	if(recordFile == NULL)
	{
		printf("\nError in file opening\n");
		exit(0);
	}
	
	printf("\n______________________________________________________\n");
	printf("\tSECONDARY INDEXING using B+ TREE..\n");
	printf("______________________________________________________\n");	
	
	
	while(1)
	{
		printf("\n______________________________________________________\n");
		printf("1. Create Index\n");
		printf("2. Find record\n");
		printf("3. Range Search\n");
		printf("4. Print tree\n");
		printf("5. Exit\n");
		printf("______________________________________________________\n\n");
		printf("Enter appropriate choice: ");
		scanf("%d", &choice);
		switch(choice)
		{
			case 1 :/* Insertion */
			
				/* Open BTFile in read write mode */
				BTFile = fopen("B+TreeFile.dat", "w+");
				if(BTFile == NULL)
				{
					printf("\nError in file opening\n");
					exit(0);
				}
	
				/* Read each record from recordFile and insert the key into BTFile which stores the B+ Tree */
				while(1)
				{
					fscanf(recordFile, "%ld", &key);
					ch = fgetc(recordFile);
					pointer = ftell(recordFile);
		
					/* Function call to insert key and pointer to record in B+ Tree */
					insert(key, pointer, BTFile);
					while(ch != '\n')
						ch = fgetc(recordFile);
	
					if(fgetc(recordFile) == EOF)
						break;
	
					fseek(recordFile, -1, SEEK_CUR);
				}
				printf("\n______All records are stored in index______\n\n");
				break;
				
			case 2: /* Searching */
				/* Open BTFile in read mode */
				BTFile = fopen("B+TreeFile.dat", "r");
				if(BTFile == NULL)
				{
					printf("\nError in file opening\n");
					exit(0);
				}
	
				printf("\nEnter key of record to be searched: ");
				scanf("%ld", &key);	
				
				/* Function call to search record corresponding to key value */
				findRecord(key, BTFile, recordFile);
				break;
			
			case 3: /* Range Search */
				/* Open BTFile in read mode */
				BTFile = fopen("B+TreeFile.dat", "r");
				if(BTFile == NULL)
				{
					printf("\nError in file opening\n");
					exit(0);
				}
	
				printf("\nEnter first key: ");
				scanf("%ld", &key);
				printf("\nEnter second key: ");
				scanf("%ld", &key2);
				
				/* Function call for range search */
				rangeSearch(key, key2, BTFile, recordFile);
				break;
				
			case 4: /* Print */
				/* Open BTFile in read mode */
				BTFile = fopen("B+TreeFile.dat", "r");
				if(BTFile == NULL)
				{
					printf("\nError in file opening\n");
					exit(0);
				}
				
				/* If file is empty then no tree is present */
				if(fgetc(BTFile) == EOF)
				{
					printf("\nTree is Empty\n");
					exit(0);
				}
				
				/* Read the structure which contains the ID of root node */
				rewind(BTFile);
				fread(r, sizeof(ROOT), 1, BTFile);
				
				/* Seek to position where root node is present and read it in root */
				fseek(BTFile, sizeof(ROOT) + (r->ID - 1) * sizeof(TNODE), SEEK_SET);
				fread(root, sizeof(TNODE), 1, BTFile);
				
				/* Function call to print tree rooted at root node */
				printTree(root, BTFile);
				break;
				
			case 5: /* Exit */
				/* Close both recordFile anf BTFile */
				fclose(BTFile);
				fclose(recordFile);
				
				/* Free memory allocated to root and r */
				free(root);
				free(r);
				
				exit(0);
				
			
			default: printf("\nEnter correct choice!!! \n");	
		}
	}
	return 0;
}


