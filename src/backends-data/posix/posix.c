/* This file is part of ESDM.                                              
 *                                                                              
 * This program is is free software: you can redistribute it and/or modify         
 * it under the terms of the GNU Lesser General Public License as published by  
 * the Free Software Foundation, either version 3 of the License, or            
 * (at your option) any later version.                                          
 *                                                                              
 * This program is is distributed in the hope that it will be useful,           
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                
 * GNU General Public License for more details.                                 
 *                                                                                 
 * You should have received a copy of the GNU Lesser General Public License        
 * along with ESDM.  If not, see <http://www.gnu.org/licenses/>.           
 */                                                                         

/**
 * @file
 * @brief A data backend to provide POSIX compatibility.
 */


#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>


#include <esdm.h>

#include "posix.h"


static void log(const char* format, ...)
{
	va_list args;
	va_start(args,format);
	vprintf(format,args);
	va_end(args);
}
#define DEBUG(msg) log("[POSIX] %-30s %s:%d\n", msg, __FILE__, __LINE__)


///////////////////////////////////////////////////////////////////////////////
// Helper and utility /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int mkfs(esdm_backend_t* backend) 
{

	posix_backend_data_t* data = (posix_backend_data_t*)backend->data;
	posix_backend_options_t* options = data->options;

	printf("mkfs: backend->(void*)data->options->target = %s\n", options->target);
	printf("\n");


	const char* tgt = options->target;

	struct stat st = {0};

	if (stat(tgt, &st) == -1)
	{
		char* root; 
		char* cont;
		char* sdat;
		char* sfra;
		
		asprintf(&root, "%s", tgt);
		asprintf(&cont, "%s/containers", tgt);
		asprintf(&sdat, "%s/shared-datasets", tgt);
		asprintf(&sfra, "%s/shared-fragments", tgt);
		
		mkdir(root, 0700);
		mkdir(cont, 0700);
		mkdir(sdat, 0700);
		mkdir(sfra, 0700);
	
		free(root);
		free(cont);
		free(sdat);
		free(sfra);
	}
}


/**
 * Similar to the command line counterpart fsck for ESDM plugins is responsible
 * to check and potentially repair the "filesystem".
 *
 */
static int fsck()
{

	return 0;
}








///////////////////////////////////////////////////////////////////////////////
// Internal Helpers ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int entry_create(const char *path)
{
	int status;
	struct stat sb;
	
	printf("entry_create(%s)\n", path);

	// ENOENT => allow to create

	status = stat(path, &sb);
	if (status == -1) {
		perror("stat");

		// write to non existing file
		int fd = open(path,	O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

		// everything ok? write and close
		if ( fd != -1 )
		{
			close(fd);
		}

		return 0;

	} else {
		// already exists
		return -1;
	}

}


static int entry_retrieve(const char *path, void **buf, size_t **count)
{
	int status;
	struct stat sb;

	printf("entry_retrieve(%s)\n", path);

	status = stat(path, &sb);
	if (status == -1) {
		perror("stat");
		// does not exist
		return -1;
	}

	//print_stat(sb);


	// write to non existing file
	int fd = open(path,	O_RDONLY | S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);


	*count = malloc(sizeof(size_t));
	*buf;

	// everything ok? write and close
	if ( fd != -1 )
	{
		// write some metadata
		*buf = (void*) malloc(sb.st_size + 1);

		char* cbuf = (char*) buf;
		cbuf[sb.st_size] = 0;

		read(fd, *buf, sb.st_size);
		close(fd);
	}




	printf("Entry content: %s\n", (char *) *buf);

	/*
	uint64_t *buf64 = (uint64_t*) buf;
	for (int i = 0; i < sb.st_size/sizeof(uint64_t); i++)
	{
		printf("idx %d: %d\n", i, buf64[i]);
	}
	*/


	return 0;
}


static int entry_update(const char *path, void *buf, size_t len)
{
	int status;
	struct stat sb;

	printf("entry_update(%s)\n", path);

	status = stat(path, &sb);
	if (status == -1) {
		perror("stat");
		return -1;
	}

	//print_stat(sb);

	// write to non existing file
	int fd = open(path,	O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

	// everything ok? write and close
	if ( fd != -1 )
	{
		// write some metadata
		write(fd, buf, len);
		close(fd);
	}

	return 0;
}


static int entry_destroy(const char *path) 
{
	int status;
	struct stat sb;

	printf("entry_destroy(%s)\n", path);

	status = stat(path, &sb);
	if (status == -1) {
		perror("stat");
		return -1;
	}

	print_stat(sb);

	status = unlink(path);
	if (status == -1) {
		perror("unlink");
		return -1;
	}

	return 0;
}




///////////////////////////////////////////////////////////////////////////////
// Container Helpers //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int container_create(esdm_backend_t* backend, esdm_container_t *container)
{
	char *path_metadata;
	char *path_container;
	struct stat sb;

	posix_backend_options_t *options = (posix_backend_options_t*) backend->data;
	const char* tgt = options->target;

	asprintf(&path_metadata, "%s/containers/%s.md", tgt, container->name);
	asprintf(&path_container, "%s/containers/%s", tgt, container->name);

	// create metadata entry
	entry_create(path_metadata);

	// create directory for datsets
	if (stat(path_container, &sb) == -1)
	{
		mkdir(path_container, 0700);
	}

	free(path_metadata);
	free(path_container);
}


static int container_retrieve(esdm_backend_t* backend, esdm_container_t *container)
{
	char *path_metadata;
	char *path_container;
	struct stat sb;

	posix_backend_options_t *options = (posix_backend_options_t*) backend->data;
	const char* tgt = options->target;


	asprintf(&path_metadata, "%s/containers/%s.md", tgt, container->name);
	asprintf(&path_container, "%s/containers/%s", tgt, container->name);


	size_t *count = NULL;
	void *buf = NULL;

	// create metadata entry
	entry_retrieve(path_metadata, &buf, &count);


	free(path_metadata);
	free(path_container);
}


static int container_update(esdm_backend_t* backend, esdm_container_t *container)
{
	char *path_metadata;
	char *path_container;
	struct stat sb;

	posix_backend_options_t *options = (posix_backend_options_t*) backend->data;
	const char* tgt = options->target;

	asprintf(&path_metadata, "%s/containers/%s.md", tgt, container->name);
	asprintf(&path_container, "%s/containers/%s", tgt, container->name);

	// create metadata entry
	entry_update(path_metadata, "abc", 3);


	free(path_metadata);
	free(path_container);
}


static int container_destroy(esdm_backend_t* backend, esdm_container_t *container) 
{
	char *path_metadata;
	char *path_container;
	struct stat sb;

	posix_backend_options_t *options = (posix_backend_options_t*) backend->data;
	const char* tgt = options->target;

	asprintf(&path_metadata, "%s/containers/%s.md", tgt, container->name);
	asprintf(&path_container, "%s/containers/%s", tgt, container->name);

	// create metadata entry
	entry_destroy(path_metadata);


	// TODO: also remove existing datasets?


	free(path_metadata);
	free(path_container);
}



///////////////////////////////////////////////////////////////////////////////
// Dataset Helpers ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int dataset_create(esdm_backend_t* backend, esdm_dataset_t *dataset)
{
	char *path_metadata;
	char *path_dataset;
	struct stat sb;


	posix_backend_options_t *options = (posix_backend_options_t*) backend->data;
	const char* tgt = options->target;

	asprintf(&path_metadata, "%s/containers/%s/%s.md", tgt, dataset->container->name, dataset->name);
	asprintf(&path_dataset, "%s/containers/%s/%s", tgt, dataset->container->name, dataset->name);

	// create metadata entry
	entry_create(path_metadata);

	// create directory for datsets
	if (stat(path_dataset, &sb) == -1)
	{
		mkdir(path_dataset, 0700);
	}

	free(path_metadata);
	free(path_dataset);
}


static int dataset_retrieve(esdm_backend_t* backend, esdm_dataset_t *dataset)
{
}


static int dataset_update(esdm_backend_t* backend, esdm_dataset_t *dataset)
{
}


static int dataset_destroy(esdm_backend_t* backend, esdm_dataset_t *dataset) 
{
}





static int fragment_retrieve(esdm_backend_t* backend, esdm_fragment_t *fragment)
{
	char *path;
	char *path_fragment;
	struct stat sb;

	posix_backend_data_t* data = (posix_backend_data_t*)backend->data;
	posix_backend_options_t* options = data->options;
	const char* tgt = options->target;


	asprintf(&path, "%s/containers/%s/%s/", tgt, fragment->dataset->container->name, fragment->dataset->name);
	asprintf(&path_fragment, "%s/containers/%s/%s/%p", tgt, fragment->dataset->container->name, fragment->dataset->name, fragment);

	printf("path: %s\n", path);
	printf("path_fragment: %s\n", path_fragment);

	// create metadata entry
	mkdir_recursive(path);
	entry_create(path_fragment);

	/*
	char *buf = NULL;
	size_t len = 6;
	entry_update(path_fragment, &buf, len);
	*/
	
	//entry_update()


	void *buf;
	size_t *count;


	entry_retrieve(path_fragment, &buf, &count);


	free(path);
	free(path_fragment);
}

static int fragment_update(esdm_backend_t* backend, esdm_fragment_t *fragment)
{
	char *path;
	char *path_fragment;
	struct stat sb;

	posix_backend_data_t* data = (posix_backend_data_t*)backend->data;
	posix_backend_options_t* options = data->options;
	const char* tgt = options->target;


	asprintf(&path, "%s/containers/%s/%s/", tgt, fragment->dataset->container->name, fragment->dataset->name);
	asprintf(&path_fragment, "%s/containers/%s/%s/%p", tgt, fragment->dataset->container->name, fragment->dataset->name, fragment);

	printf("path: %s\n", path);
	printf("path_fragment: %s\n", path_fragment);

	// create metadata entry
	mkdir_recursive(path);
	entry_create(path_fragment);

	/*
	char *buf = NULL;
	size_t len = 6;
	entry_update(path_fragment, &buf, len);
	*/
	
	entry_update(path_fragment, fragment->data, fragment->bytes);

	//entry_update()

	size_t *count = NULL;
	void *buf = NULL;

	entry_retrieve(path_fragment, &buf, &count);


	free(path);
	free(path_fragment);
}









///////////////////////////////////////////////////////////////////////////////
// ESDM Callbacks /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int posix_backend_performance_estimate(esdm_backend_t* backend) 
{
	DEBUG("Calculating performance estimate");

	posix_backend_data_t* data = (posix_backend_data_t*)backend->data;
	posix_backend_options_t* options = data->options;

	printf("perf_estimate: backend->(void*)data->options->target = %s\n", options->target);
	printf("\n");

	return 0;
}


static int posix_create(esdm_backend_t* backend, char * name) 
{
	DEBUG("Create");


	// check if container already exists

	struct stat st = {0};
	if (stat("_esdm-fs", &st) == -1)
	{
		mkdir("_esdm-fs/containers", 0700);
	}


    //#include <unistd.h>
    //ssize_t pread(int fd, void *buf, size_t count, off_t offset);
    //ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

	return 0;
}


/**
 *	
 *	handle
 *	mode
 *	owner?	
 *
 */
static int posix_open(esdm_backend_t* backend) 
{
	DEBUG("Open");
return 0;
}

static int posix_write(esdm_backend_t* backend) 
{
	DEBUG("Write");
	return 0;
}

static int posix_read(esdm_backend_t* backend) 
{
	DEBUG("Read");
	return 0;
}

static int posix_close(esdm_backend_t* backend) 
{
	DEBUG("Close");
	return 0;
}



static int posix_allocate(esdm_backend_t* backend) 
{
	DEBUG("Allocate");
	return 0;
}


static int posix_update(esdm_backend_t* backend) 
{
	DEBUG("Update");
	return 0;
}


static int posix_lookup(esdm_backend_t* backend) 
{
	DEBUG("Lookup");
	return 0;
}



///////////////////////////////////////////////////////////////////////////////
// ESDM Module Registration ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static esdm_backend_t backend_template = {
///////////////////////////////////////////////////////////////////////////////
// WARNING: This serves as a template for the posix plugin and is memcpied!  //
///////////////////////////////////////////////////////////////////////////////
	.name = "POSIX",
	.type = ESDM_TYPE_DATA,
	.version = "0.0.1",
	.data = NULL,
	.callbacks = {
		NULL, // finalize
		posix_backend_performance_estimate, // performance_estimate

		posix_create, // create
		posix_open, // open
		posix_write, // write
		posix_read, // read
		posix_close, // close

		// Metadata Callbacks
		NULL, // lookup

		// ESDM Data Model Specific
		NULL, // container create
		NULL, // container retrieve
		NULL, // container update
		NULL, // container delete

		NULL, // dataset create
		NULL, // dataset retrieve
		NULL, // dataset update
		NULL, // dataset delete

		NULL, // fragment create
		fragment_retrieve, // fragment retrieve
		fragment_update, // fragment update
		NULL, // fragment delete
	},
};

/**
* Initializes the POSIX plugin. In particular this involves:
*
*	* Load configuration of this backend
*	* Load and potentially calibrate performance model
*
*	* Connect with support services e.g. for technical metadata
*	* Setup directory structures used by this POSIX specific backend
*
*	* Populate esdm_backend_t struct and callbacks required for registration
*
* @return pointer to backend struct
*/
esdm_backend_t* posix_backend_init(void* init_data) {
	
	DEBUG("Initializing POSIX backend");

	esdm_backend_t* backend = (esdm_backend_t*) malloc(sizeof(esdm_backend_t));
	memcpy(backend, &backend_template, sizeof(esdm_backend_t));

	backend->data = (void*) malloc(sizeof(posix_backend_data_t));
	posix_backend_data_t* data = (posix_backend_data_t*) backend->data;
	posix_backend_options_t* options = (posix_backend_options_t*) init_data;
	data->options = options;


	printf("[POSIX] Backend config: type=%s, name=%s, target=%s\n", options->type, options->name, options->target);


	// valid refs for backend, data, options available now


	data->other = 47;


	// todo check posix style persitency structure available?
	mkfs(backend);	






	return backend;

}

/**
* Initializes the POSIX plugin. In particular this involves:
*
*/
int posix_finalize(esdm_backend_t* backend)
{
	DEBUG("POSIX finalize");

	return 0;
}

