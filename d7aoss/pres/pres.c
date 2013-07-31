/*
 * trans.c
 *
 *  Created on: 05-mai-2013
 *      Author: Maarten Weyn
 */


#include "pres.h"
#include "../trans/trans.h"



uint8_t fs_open(file_handler *fh, file_system_type fst, uint8_t file_id, file_system_user user, file_system_access_type access_type)
{
	if (fst != file_system_type_isfb)
	{
		//TODO: implement isfbs and gfb
		return 1;
	}

	// fst == file_system_type_isfb

	if (file_id >= FILESYSTEM_IFSB_FILES)
		return 1;

	file_info *fi = (void*) (FILESYSTEM_FILE_INFO_START_ADDRESS + (sizeof(file_info) * file_id));

	uint8_t permission_mask = 0;
	// TODO: validate correct user
	if (user == file_system_user_user)
	{
		permission_mask |= PERMISSION_CODE_USER_MASK;
	} else if (user == file_system_user_guest)
	{
		permission_mask |= PERMISSION_CODE_GUEST_MASK;
	}

	if (access_type == file_system_access_type_read)
	{
		permission_mask &= PERMISSION_CODE_READ_MASK;
	} else if (access_type == file_system_access_type_write)
	{
		permission_mask &= PERMISSION_CODE_WRITE_MASK;
	} else if (access_type == file_system_access_type_run)
	{
		permission_mask &= PERMISSION_CODE_RUN_MASK;
	}

	if ((user != file_system_user_root) && !(fi->permissions & permission_mask))
		return 2;

	fh->file_info = fi;
	fh->file = (void*) (FILESYSTEM_FILES_START_ADDRESS + fi->offset);

	return 0;
}

void pres_init()
{
	file_handler network_settings_fh;
	uint8_t result = fs_open(&network_settings_fh, file_system_type_isfb, 0x00, file_system_user_root, file_system_access_type_read);
	trans_init();
}
