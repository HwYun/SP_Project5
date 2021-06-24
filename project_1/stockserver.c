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
void print_item(itemptr root, char* buf);

itemptr stock_echo(int connfd, itemptr root);
void echo(int connfd);


int main(int argc, char **argv) 
{
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
	char client_hostname[MAXLINE], client_port[MAXLINE];
	fd_set reads, temps;
	struct timeval timeout;
	int fd_max;

	itemptr root = NULL;
	int text_fd;
	int nbytes;
	char buf[512];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]); // bind까지 끝남.

	FD_ZERO(&reads); // fd_set 초기화
	FD_SET(listenfd, &reads); // listen 소켓 관리대상으로 지정
	fd_max = listenfd;
	
	/* 이진 트리 구성 */
	if ((text_fd=open("stock.txt", O_RDONLY)) < 0){
		perror("open");
		exit(1);
	}
	if((nbytes=read(text_fd, buf, sizeof(buf))) < 0){
		perror("read");
		exit(1);
	}

	char tmp_line[100] = {0,};
	int new_line_pos=0;
	int tmp_ID, tmp_left_stock, tmp_price;
	char *tmp_ptr = NULL;

	for(int i=0 ; i<nbytes ; i++){
		if(buf[i] == EOF || buf[i]== '\n'){
			for(int j=0 ; j<100 ; j++) tmp_line[j] = 0;
			strncpy(tmp_line, buf+new_line_pos, i - new_line_pos);
			new_line_pos = i;
			tmp_ptr = strtok(tmp_line, " ");
			tmp_ID = atoi(tmp_ptr);
			tmp_ptr = strtok(NULL, " ");
			tmp_left_stock = atoi(tmp_ptr);
			tmp_ptr = strtok(NULL, " ");
			tmp_price = atoi(tmp_ptr);
			root = insert_item(root, tmp_ID, tmp_left_stock, tmp_price);
			//printf("%d %d %d\n",tmp_ID, tmp_left_stock, tmp_price);
		}
		// printf("%c", buf[i]);
	}
	
	Close(text_fd);

	// epoll 관련 변수들 선언
	//struct epoll_event* ep_events_buf;
	//struct epoll_event userevent;
	//int epfd, event_cnt;
	int i;

	//epfd = epoll_create(1000);
	//ep_events_buf = malloc(sizeof(struct epoll_event) * 1000);

	//userevent.events = EPOLLIN;
	//userevent.data.fd = listenfd;
	//epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &userevent);


	int sel_result;
	while (1) {
		temps = reads;
		timeout.tv_sec = 3;
		timeout.tv_usec = 3000;

		sel_result = select(fd_max+1, &temps, 0, 0, &timeout);

		if(sel_result == -1) break;
		else if(sel_result == 0) continue;
		else{
			for(i=0 ; i<fd_max+1 ; i++){
				if(FD_ISSET(i, &temps)){ 
					if(i == listenfd){
						clientlen = sizeof(struct sockaddr_storage); 
						connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
						FD_SET(connfd, &reads);
						if(fd_max<connfd) fd_max = connfd;
						Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
								client_port, MAXLINE, 0);
						printf("Connected to (%s, %s)\n", client_hostname, client_port);
					}
					else{
						root=stock_echo(i, root);
						FD_CLR(i, &reads);
						Close(i);

					}
				}
			}
		}
		//Close(connfd);
		
		/*
		// 무한 대기하며 epfd중에 변화가 있는 fd 확인. 확인 후 event 버퍼에 넣음.
		event_cnt = epoll_wait(epfd, ep_events_buf, 1000, -1);

		if(event_cnt ==-1) 
		{
			puts("wait() error!\n");
			break;
		}

		// 확인된 변화의 갯수만큼 for문
		for(i=0; i<event_cnt; i++)
		{
			// 만약 이벤트가 생겼고 그 fd가 서버라면 connection request 
			if(ep_events_buf[i].data.fd == listenfd)
			{
				clientlen = sizeof(struct sockaddr_storage); 
				connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
				Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
						client_port, MAXLINE, 0);
				printf("Connected to (%s, %s)\n", client_hostname, client_port);

				// 클라이언트의 fd를 등록해주는 과정
				userevent.events=EPOLLIN;
				userevent.data.fd = connfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &userevent);

			}
			// 서버가 아니라면 -> 클라이언트의 변화
			else{
				root=stock_echo(ep_events_buf[i].data.fd, root);
			}
		}
		*/

		char tmp_text_buf[MAXLINE];
		for(int j=0 ; j< MAXLINE ; j++) tmp_text_buf[j] = 0;
		tmp_text_buf[0] = ' ';

		/* text file update */
		
		if ((text_fd=open("stock.txt", O_WRONLY)) < 0){
			perror("open");
			exit(1);
		}
		print_item(root, tmp_text_buf);
		if(write(text_fd, tmp_text_buf+1, strlen(tmp_text_buf) - 1) < 0){
			perror("write");
			exit(1);
		}
		Close(text_fd);
		/*
		print_item(root, tmp_text_buf);
		FILE *fp = fopen("stock.txt", "w");
		fprintf(fp, "%s", tmp_text_buf);
		fclose(fp);
		*/
	}



	Close(listenfd);
	//close(epfd);
	free_item(root);
	exit(0);
	return 0;
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
		Sem_init(&(root->mutex), 0, 1);
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
		P(&(root->mutex));
		root->left_stock = left_stock;
		V(&(root->mutex));
	}
	else if(root->ID > ID)
		root->leftChild = update_item(root->leftChild, ID, left_stock);
	else 
		root->rightChild = update_item(root->rightChild, ID, left_stock);

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

void print_item(itemptr root, char *buf){
	if(root == NULL) return;

	char tmp_buf[MAXLINE]= {-1 ,};

	sprintf(tmp_buf,"%d %d %d\n", root->ID, root->left_stock, root->price);
	strcat(buf, tmp_buf);
	print_item(root->leftChild, buf);
	print_item(root->rightChild, buf);	
}

itemptr stock_echo(int connfd, itemptr root){
	int n; 
	char buf[MAXLINE]; 
	rio_t rio;

	char *tmp_ptr=NULL;
	char tmp_buf[MAXLINE];
	int tmp_ID, tmp_stock_num;
	itemptr tmp_item=NULL;
	Rio_readinitb(&rio, connfd);
	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		printf("server received %d bytes\n", n);
		for(int i=0 ; i<MAXLINE ; i++) tmp_buf[i] = 0;
		strncpy(tmp_buf, buf, n-1);

		for(int i=0 ; i<MAXLINE ; i++) buf[i] = 0;
		if(tmp_buf[0]=='s'){
			if(tmp_buf[1]=='h'){ // show
				print_item(root, buf);
			}
			else if(tmp_buf[1]=='e'){ // sell
				tmp_ptr=strtok(tmp_buf, " ");
				tmp_ptr=strtok(NULL, " ");
				tmp_ID = atoi(tmp_ptr);
				tmp_ptr=strtok(NULL, " ");
				tmp_stock_num = atoi(tmp_ptr);

				tmp_item = search_item(root, tmp_ID);

				root = update_item(root, tmp_ID, tmp_item->left_stock + tmp_stock_num);

				strcpy(buf, "[sell] success\n");
			}
		}
		else if(tmp_buf[0]=='b'){
			// buy
			tmp_ptr=strtok(tmp_buf, " ");
			tmp_ptr=strtok(NULL, " ");
			tmp_ID = atoi(tmp_ptr);
			tmp_ptr=strtok(NULL, " ");
			tmp_stock_num = atoi(tmp_ptr);

			tmp_item = search_item(root, tmp_ID);

			if(tmp_item->left_stock < tmp_stock_num){
				strcpy(buf, "Not enough left stock\n");
			}
			else{
				root = update_item(root, tmp_ID, tmp_item->left_stock - tmp_stock_num);
				strcpy(buf, "[buy] success\n");
			}
		}
		else if(tmp_buf[0] == 'e')
			strcpy(buf, tmp_buf);
		Rio_writen(connfd, buf, MAXLINE);
	}
	//FD_CLR(connfd, &reads);
	//Close(connfd);
	return root;
}
