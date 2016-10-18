# container
rough implementation of containers

Two part post that explains this:
http://tejom.github.io/c/linux/containers/docker/2016/10/04/containers-from-scratch-pt1.html
http://tejom.github.io/c/linux/containers/docker/networking/2016/10/08/containers-from-scratch-pt2-networking.html

How to use:
Copied from part 2 post.

Run make. This will create a executable called main.

The program takes to sets of arguments, an ip address for the container and a command to execute.

sudo ./main 172.16.0.30 bash

Will run bash in the container.

sudo ./main 172.16.0.30 nc -l -p 8000

Will run the nc command with the arguments.
