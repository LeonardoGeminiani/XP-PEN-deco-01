#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <linux/input.h>
#include <signal.h>
#include <stdlib.h>

#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define VENDOR 0x28bd
#define PRODUCT 0x0905

static char *strmerge(const char *const s1, const char *const s2)
{
	const size_t n1 = s1 ? strlen(s1) : 0;
	const size_t n2 = s2 ? strlen(s2) : 0;
	char *s;

	if (!n1 && !n2) {
		errno = EINVAL;
		return NULL;
	}

	s = malloc(n1 + n2 + 1);
	if (!s) {
		errno = ENOMEM;
		return NULL;
	}

	if (n1)	memcpy(s, s1, n1);
	if (n2)	memcpy(s + n1, s2, n2);
	s[n1 + n2] = 0;

	return s;
}

static int read_hex(const char *const filename)
{
	FILE *in;
	unsigned int value;

	in = fopen(filename, "rb");
	if (!in)
		return -1;

	if (fscanf(in, "%x", &value) == 1) {
		fclose(in);
		return (int)value;
	}

	fclose(in);
	return -1;
}

static int vendor_id(const char *const event)
{
	if (event && *event) {
		char filename[256];
		int	result;

		result = snprintf(filename, sizeof(filename), "/sys/class/input/%s/device/id/vendor", event);
		if (result < 1 || result >= sizeof(filename))
			return -1;

		return read_hex(filename);
	}
	return -1;
}

static int product_id(const char *const event)
{
	if (event && *event) {
		char filename[256];
		int	result;

		result = snprintf(filename, sizeof(filename), "/sys/class/input/%s/device/id/product", event);
		if (result < 1 || result >= sizeof(filename))
			return -1;

		return read_hex(filename);
	}
	return -1;
}

char *find_event(const int vendor, const int product)
{
	DIR	*dir;
	struct dirent *cache, *entry;
	char *name;
	long maxlen;
	int result;

	maxlen = pathconf("/sys/class/input/", _PC_NAME_MAX);
	if (maxlen == -1L)
		return NULL;

	dir = opendir("/sys/class/input");
	if (!dir)
		return NULL;

	cache = malloc(offsetof(struct dirent, d_name) + maxlen + 1);
	if (!cache) {
		closedir(dir);
		errno = ENOMEM;
		return NULL;
	}

	while (1) {

		entry = NULL;
		result = readdir_r(dir, cache, &entry);
		if (result) {
			free(cache);
			closedir(dir);
			errno = result;
			return NULL;
		}

		if (!entry) {
			free(cache);
			closedir(dir);
			errno = ENOENT;
			return NULL;
		}
        
		if (vendor_id(entry->d_name) == vendor &&
		    product_id(entry->d_name) == product) {
			name = strmerge("/dev/input/", entry->d_name);
			free(cache);
			closedir(dir);
			if (name)
				return name;
			errno = ENOMEM;
			return NULL;
		}
	}
}

void INThandler(){
    exit(0);
}

int main()
{
    char *devname;

    devname = find_event(VENDOR, PRODUCT);
    if (!devname) exit(1);

    int device = open(devname, O_RDONLY);
    struct input_event ev;

    signal(SIGINT, INThandler);

    while(1)
    {
        read(device,&ev, sizeof(ev));
        if(ev.type == 1 && ev.value == 1){
            unsigned char id = ev.code-0x100;
            
            switch (id)
            {
				case 0: // first button
					system("firefox & disown");
					break;
				case 1:
					system("nautilus & disown");
					break;
				case 2:
					system("/opt/JetBrains-Rider/bin/rider.sh & disown");
					break;
				
				default:
					fprintf(stderr, "%s", "Error, Invalid Value");
					break;
            }

            printf("%d\n", id);
        }
    }
}