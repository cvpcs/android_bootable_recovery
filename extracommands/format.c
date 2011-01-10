/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "roots.h"

int format_main(int argc, char** argv)
{
    if(argc<=1)
    {
	puts("usage: format <partition>");
	puts("   where <partition> is one of:");
	puts("     \"/cache\"");
	puts("     \"/data\"");
	puts("     \"/system\"");
	return(1);
    }

    const char* root = argv[1];
    printf("Formatting %s\n",root);

    int status = format_volume(root);

    if (status != 0) {
        printf("Can't format %s\n", root);
        return status;
    }

    return(status);
}
