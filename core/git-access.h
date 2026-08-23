// SPDX-License-Identifier: GPL-2.0
#ifndef GITACCESS_H
#define GITACCESS_H

#include "git2.h"
#include "filterpreset.h"
#include <string>

struct dive_log;
struct git_oid;
struct git_repository;
struct divelog;

#define CLOUD_HOST_US "ssrf-cloud-us.subsurface-divelog.org"  // preferred (faster/bigger) server in the US
#define CLOUD_HOST_U2 "ssrf-cloud-u2.subsurface-divelog.org"  // secondary (older) server in the US
#define CLOUD_HOST_EU "ssrf-cloud-eu.subsurface-divelog.org"  // preferred (faster/bigger) server in Germany
#define CLOUD_HOST_E2 "ssrf-cloud-e2.subsurface-divelog.org"  // secondary (older) server in Germany
#define CLOUD_HOST_PATTERN "ssrf-cloud-..\\.subsurface-divelog\\.org"
#define CLOUD_HOST_GENERIC "cloud.subsurface-divelog.org"

enum remote_transport { RT_LOCAL, RT_HTTPS, RT_SSH, RT_OTHER };

extern bool git_local_only;
extern bool git_remote_sync_successful;
extern void clear_git_id();
void set_git_update_cb(int(*)(const char *));
int git_storage_update_progress(const char *text);
int get_authorship(git_repository *repo, git_signature **authorp);

struct git_info {
	std::string url;
	std::string branch;
	std::string username;
	std::string localdir;
	struct git_repository *repo;
	unsigned is_subsurface_cloud:1;
	enum remote_transport transport;
	git_info();
	~git_info();
};

// AI-generated (Claude)
struct git_provenance {
	std::string repository;
	std::string branch;
	std::string commit;

	bool empty() const { return repository.empty() || branch.empty() || commit.empty(); }
};

enum class git_save_preflight_status {
	allowed,
	destination_missing,
	replacement_confirmation_required,
	error
};

struct git_save_preflight_result {
	git_save_preflight_status status;
	std::string error;
};

extern git_provenance loaded_git_provenance;
extern void set_git_provenance(const struct git_info *, const struct git_oid *);
extern std::string canonical_git_repository(const struct git_info *);
extern git_save_preflight_result preflight_git_save(const struct git_info *);
extern std::string get_sha(git_repository *repo, const std::string &branch);
extern std::string get_local_dir(const std::string &, const std::string &);
extern bool is_git_repository(const char *filename, struct git_info *info);
extern bool open_git_repository(struct git_info *info);
extern bool remote_repo_uptodate(const char *filename, struct git_info *info);
extern int sync_with_remote(struct git_info *);
extern int git_save_dives(struct git_info *, bool select_only);
// AI-generated (Claude): Explicit whole-log replacement entry points.
extern int replace_dives(const char *filename);
extern int git_replace_dives(struct git_info *);
extern int refresh_remote_for_replacement(struct git_info *, struct git_oid *remote_tip);
extern int push_git_replacement(struct git_info *, const struct git_oid *commit_id);
extern int git_load_dives(struct git_info *, struct divelog *log);
extern int do_git_save(struct git_info *, bool select_only, bool create_empty);
extern int git_create_local_repo(const std::string &filename);

#endif // GITACCESS_H
