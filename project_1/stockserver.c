/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct _item* itemptr;

typedef struct _item{
	int ID;
	int left_stock;
	int price;
	int readcnt;
	sem_t mutex;
	itemptr leftChild;
	itemptr rightChild;
}item;

itemptr insert_item(itemptr root, int ID, int left_stock, int price);
itemptr update_item(itemptr root, int ID, int left_stock);
itemptr search_item(itemptr root, int ID);
void free_item(itemptr root);
void print_item(itemptr root);

void echo(int connfd);

int main(int argc, char **argv) 
{
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
	char client_hostname[MAXLINE], client_port[MAXLINE];

	itemptr root = NULL;
	int text_fd;
	int nbytes;
	char buf[512];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	
	/* 이진 트리 구성 */
	if ((text_fd=open("stock.txt", O_RDONLY)) < 0){
		perror("open");
		exit(1);
	}
	if((nbytes=read(text_fd, buf, sizeof(buf))) < 0){
		perror("read");
		exit(1);
	}

	char tmp_line[20] = {0,};
	int new_line_pos=0;
	int tmp_ID, tmp_left_stock, tmp_price;
	char *tmp_ptr = NULL;

	for(int i=0 ; i<nbytes ; i++){
		if(buf[i] == EOF || buf[i]== '\n'){
			strncpy(tmp_line, buf+new_line_pos, i - new_line_pos);
			new_line_pos = i;
			tmp_ptr = strtok(tmp_line, " ");
			tmp_ID = atoi(tmp_ptr);
			tmp_ptr = strtok(NULL, " ");
			tmp_left_stock = atoi(tmp_ptr);
			tmp_ptr = strtok(NULL, " ");
			tmp_price = atoi(tmp_ptr);
			root = insert_item(root, tmp_ID, tmp_left_stock, tmp_price);
			// printf("%d %d %d\n",tmp_ID, tmp_left_stock, tmp_price);
		}
		// printf("%c", buf[i]);
	}
	
	print_item(root);
	free_item(root);
	Close(text_fd);

	while (1) {
		clientlen = sizeof(struct sockaddr_storage); 
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
				client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		echo(connfd);
		Close(connfd);
	}
	exit(0);
}
/* $end echoserverimain */

itemptr insert_item(itemptr root, int ID, int left_stock, int price){ // 트리에 item 노드 삽입 후 root를 반환.
	if(root == NULL){
		root = (itemptr)malloc(sizeof(item));
		root->leftChild = NULL;
		root->rightChild = NULL;
		root->ID = ID;
		root->left_stock = left_stock;
		root->price = price;
		return root;
	}

	if(root->ID > ID)
		root->leftChild = insert_item(root->leftChild, ID, left_stock, price);
	else
		root->rightChild = insert_item(root->rightChild, ID, left_stock, price);

	return root;

}

itemptr update_item(itemptr root, int ID, int left_stock){
	if(root == NULL) return NULL;

	if(root->ID == ID){
		root->left_stock = left_stock;
	}
	else if(root->ID > ID)
		root->leftChild = update_item(root->leftChild, ID, left_stock);
	else root->rightChild = update_item(root->rightChild, ID, left_stock);

	return root;
}

itemptr search_item(itemptr root, int ID){ // ID에 해당하는 노드의 주소를 반환
	if(root == NULL) return NULL;

	if(root->ID == ID)
		return root;
	else if(root->ID > ID)
		return search_item(root->leftChild, ID);
	else
		return search_item(root->rightChild, ID);
}

void free_item(itemptr root){
	if(root == NULL) return;

	free_item(root->leftChild);
	free_item(root->rightChild);
	free(root);
}

void print_item(itemptr root){
	if(root == NULL) return;

	printf("%d %d %d\n", root->ID, root->left_stock, root->price);
	print_item(root->leftChild);
	print_item(root->rightChild);
}

