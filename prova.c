#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int max_fd = 0, count = 0;

int ids[65536];
char *msg[65536];
fd_set rfds, wfds, fds;
char buf_read[1001], buf_write[42];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	notify_other(int author, char *str)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, str, strlen(str), 0);
	}
}


void	send_msg(int fd)
{
	char *mnsg;

	while (extract_message(&(msg[fd]), &mnsg))
	{
		sprintf(buf_write, "client %d: %s", ids[fd], mnsg);
		notify_other(fd, buf_write);
		notify_other(fd, msg[fd]);
		free(mnsg);
	}
}


int main(int ac, char **av) {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr; 

	if(ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		write(2, "Fatal error\n", 12);
		exit(1); 
	}
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
	bzero(&servaddr, sizeof(servaddr)); 
	max_fd = sockfd;
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		write(2, "Fatal error\n", 12);
		exit(1); 
	}

	if (listen(sockfd, 10) != 0) {
		write(2, "Fatal error\n", 12);
		exit(1); 
	}

	while(1){

		rfds = wfds = fds;

		if(select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
		{
			write(2, "Fatal error\n", 12);
			exit(1); 	
		}
		
		for(int fd = 0; fd <= max_fd; fd++)
		{
			if(FD_ISSET(fd, &rfds))
			{
				if(fd == sockfd)
				{
					len = sizeof(servaddr);
					connfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
					if (connfd < 0) { 
						continue;
					}
					if(connfd > max_fd)
						max_fd = connfd;
					ids[connfd] = count++;
					msg[connfd] = NULL;
					FD_SET(connfd, &fds);
					sprintf(buf_write, "server: client %d just arrived\n", ids[connfd]);
					notify_other(connfd, buf_write);
					break;

				}
				else
				{
					int read_bytes = recv(fd, buf_read, 1000, 0);
					if (read_bytes <= 0)
					{
						sprintf(buf_write, "server: client %d just left\n", ids[fd]);
						notify_other(fd, buf_write);
						FD_CLR(fd, &fds);
						free(msg[fd]);
						close(fd);
						break;
					} 
					buf_read[read_bytes] = '\0';
					msg[fd] = str_join(msg[fd], buf_read);
					send_msg(fd);
				}
			}
		}


	}

}

