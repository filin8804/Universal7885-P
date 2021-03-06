/*
 * sdp_ioctl.c
 *
 */
#include <linux/uaccess.h>
#include "../ext4.h"

#if defined(CONFIG_EXT4CRYPT_SDP) || defined(CONFIG_DDAR)
#include "../fscrypt_knox_private.h"
#endif

#ifdef CONFIG_EXT4CRYPT_SDP
#include "fscrypto_sdp_dek_private.h"
#include "fscrypto_sdp_ioctl.h"
#endif

int fscrypt_sdp_ioctl_get_sdp_info(struct inode *inode, unsigned long arg)
{
	struct dek_arg_sdp_info req;
	int result = 0;

	if (ext4_encryption_info(inode) == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		return -EOPNOTSUPP;
	}

	if (copy_from_user(&req,
			(struct dek_arg_get_sdp_info __user *)arg, sizeof(req))) {
		DEK_LOGE("can't copy from user\n");
		memset(&req, 0, sizeof(struct dek_arg_sdp_info));
		result = -EFAULT;
	} else {
		struct ext4_crypt_info *ci = ext4_encryption_info(inode);

		req.engine_id = -1;
		req.type = -1;
		req.sdp_enabled = 1;

		if (ci->ci_sdp_info) {
			DEK_LOGD("ei->i_crypt_info->sdp_flags: %x\n", ci->ci_sdp_info->sdp_flags);

			if (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE) {
				req.is_sensitive = 1;
				req.engine_id = ci->ci_sdp_info->engine_id;
				req.type = ci->ci_sdp_info->sdp_dek.type;
			}
			if (ci->ci_sdp_info->sdp_flags & SDP_IS_CHAMBER_DIR)
				req.is_chamber = 1;
		} else {
			req.is_sensitive = 0;
			req.is_chamber = 0;
		}

		if (copy_to_user((void __user *)arg, &req, sizeof(req)))
			return -EFAULT;
	}

	return result;
}

int fscrypt_sdp_ioctl_set_sensitive(struct inode *inode, unsigned long arg)
{
	dek_arg_set_sensitive_t req;
	int result = 0;

	if (ext4_encryption_info(inode) == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		struct ext4_crypt_info *ci = ext4_encryption_info(inode);

		if (ci->ci_sdp_info &&
				(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
			DEK_LOGE("already sensitive file\n");
			return 0;
		}
		if (S_ISDIR(inode->i_mode) && !is_root()) {
#ifdef CONFIG_SDP_KEY_DUMP
			if (get_sdp_sysfs_key_dump()) {
				DEK_LOGD("Temporarily allowed to process not vold.");
			} else {
#endif
			DEK_LOGE("Only vold as root process can set sensitive directory\n");
			return -EPERM;
#ifdef CONFIG_SDP_KEY_DUMP
			}
#endif
		}

		memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
		if (copy_from_user(&req,
				(dek_arg_set_sensitive_t __user *)arg, sizeof(req))) {
			DEK_LOGE("can't copy from user\n");
			memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
			result = -EFAULT;
		} else {
			int rc = fscrypt_sdp_set_sensitive(req.engine_id, inode);

			if (rc) {
				DEK_LOGE("failed to set sensitive rc(%d)\n", rc);
				memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
				return -EFAULT;
			}
			memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
		}
	}
	return result;
}

int fscrypt_sdp_ioctl_set_protected(struct inode *inode, unsigned long arg)
{
	int result = 0;

	if (ext4_encryption_info(inode) == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		int rc;

		if (S_ISDIR(inode->i_mode) && !is_root()) {
#ifdef CONFIG_SDP_KEY_DUMP
			if (get_sdp_sysfs_key_dump()) {
				DEK_LOGD("Temporarily allowed to process not vold.");
			} else {
#endif
			DEK_LOGE("Only vold as root process can set protected directory\n");
			return -EPERM;
#ifdef CONFIG_SDP_KEY_DUMP
			}
#endif
		}

		rc = fscrypt_sdp_set_protected(inode);
		if (rc) {
			DEK_LOGE("failed to set protected rc(%d)\n", rc);
			result = -EFAULT;
		}
	}
	return result;
}

int fscrypt_sdp_ioctl_add_chamber_directory(struct inode *inode, unsigned long arg)
{
	int result = 0;

	if (ext4_encryption_info(inode) == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		dek_arg_add_chamber_t req;
		struct ext4_crypt_info *ci = ext4_encryption_info(inode);

		if (!S_ISDIR(inode->i_mode)) {
			DEK_LOGE("Not directory\n");
			return -EOPNOTSUPP;
		}

		if (ci->ci_sdp_info &&
				ci->ci_sdp_info->sdp_flags & SDP_IS_CHAMBER_DIR) {
			DEK_LOGE("Already chamber directory\n");
			return 0;
		}
		if (!is_root()) {
			DEK_LOGE("Permission denied: only epm process can call this\n");
			return -EPERM;
		}

		memset(&req, 0, sizeof(dek_arg_add_chamber_t));
		if (copy_from_user(&req,
				(dek_arg_add_chamber_t __user *)arg, sizeof(req))) {
			DEK_LOGE("can't copy from user\n");
			memset(&req, 0, sizeof(dek_arg_add_chamber_t));
			result = -EFAULT;
		} else {
			int rc = fscrypt_sdp_add_chamber_directory(req.engine_id, inode);

			if (rc) {
				DEK_LOGE("failed to add chamber rc(%d)\n", rc);
				memset(&req, 0, sizeof(dek_arg_add_chamber_t));
				return -EFAULT;
			}
			memset(&req, 0, sizeof(dek_arg_add_chamber_t));
		}
	}
	return result;
}

int fscrypt_sdp_ioctl_remove_chamber_directory(struct inode *inode)
{
	int result = 0;

	if (ext4_encryption_info(inode) == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		int rc;
		struct ext4_crypt_info *ci = ext4_encryption_info(inode);

		if (!ci->ci_sdp_info ||
				!(ci->ci_sdp_info->sdp_flags & SDP_IS_CHAMBER_DIR)) {
			DEK_LOGE("Not chamber directory\n");
			return 0;
		}
		if (!is_root()) {
			DEK_LOGE("Permission denied: only epm process can call this\n");
			return -EPERM;
		}
		rc = fscrypt_sdp_remove_chamber_directory(inode);
		if (rc) {
			DEK_LOGE("failed to remove chamber rc(%d)\n", rc);
			result = -EFAULT;
		}
	}
	return result;
}

/*
 * -ENOTTY will be returned if this ioctl is not related to SDP
 */
int fscrypt_sdp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file_inode(filp);

	switch (cmd) {
	case EXT4_IOC_GET_SDP_INFO:
		return fscrypt_sdp_ioctl_get_sdp_info(inode, arg);
	case EXT4_IOC_SET_SDP_POLICY:
		return 0;
	case EXT4_IOC_SET_SENSITIVE:
		return fscrypt_sdp_ioctl_set_sensitive(inode, arg);
	case EXT4_IOC_SET_PROTECTED:
		return fscrypt_sdp_ioctl_set_protected(inode, arg);
	case EXT4_IOC_ADD_CHAMBER:
		return fscrypt_sdp_ioctl_add_chamber_directory(inode, arg);
	case EXT4_IOC_REMOVE_CHAMBER:
		return fscrypt_sdp_ioctl_remove_chamber_directory(inode);
	default:
		return -ENOTTY;
	}
}
