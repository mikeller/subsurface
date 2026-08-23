// SPDX-License-Identifier: GPL-2.0
#include "testgitstorage.h"
#include "git2.h"

#include "core/device.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/subsurface-string.h"
#include "core/format.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPrefProxy.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/git-access.h"

#include <QDir>
#include <QTextStream>
#include <QNetworkProxy>
#include <QTextCodec>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif
#include <QTemporaryDir>

// provide a declaration for a local helper function in git-access.cpp
void delete_remote_branch(git_repository *repo, const std::string &remote, const std::string &branch);

Q_DECLARE_METATYPE(std::string);

std::string email;
std::string gitUrl;
std::string cloudTestRepo;
std::string localCacheDir;
std::string localCacheRepo;
std::string randomBranch;

static void moveDir(const std::string &oldName, const std::string &newName)
{
	QDir oldDir(oldName.c_str());
	QDir newDir(newName.c_str());
	QCOMPARE(newDir.removeRecursively(), true);
	QCOMPARE(oldDir.rename(oldName.c_str(), newName.c_str()), true);
}

static void localRemoteCleanup()
{
	// cleanup the local cache dir
	struct git_info info;
	QDir localCacheDirectory(localCacheDir.c_str());
	QCOMPARE(localCacheDirectory.removeRecursively(), true);

	// when this is first executed, we expect the branch not to exist on the remote server;
	// if that's true, this will print a harmless error to stderr
	is_git_repository(cloudTestRepo.c_str(), &info) && open_git_repository(&info);

	// this odd comparison is used to tell that we were able to connect to the remote repo;
	// in the error case we get the full cloudTestRepo name back as "branch"
	if (info.branch != randomBranch || info.repo == nullptr) {
		// dang, we weren't able to connect to the server - let's not fail the test
		// but just give up
		QSKIP("wasn't able to connect to server");
	}

	// force delete any remote branch of that name on the server (and ignore any errors)
	delete_remote_branch(info.repo, info.url, info.branch);

	// and since this will have created a local repo, remove that one, again so the tests start clean
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
}

// AI-generated (Claude)
static git_save_preflight_result savePreflight(const std::string &filename)
{
	git_info info;
	if (!is_git_repository(filename.c_str(), &info))
		return { git_save_preflight_status::error, "not a git repository name" };
	return preflight_git_save(&info);
}

static void createTestBranches(git_repository *repo)
{
	git_reference *mainRef = nullptr;
	git_commit *mainCommit = nullptr;
	git_reference *otherRef = nullptr;
	QCOMPARE(git_branch_lookup(&mainRef, repo, "main", GIT_BRANCH_LOCAL), 0);
	QCOMPARE(git_reference_peel(reinterpret_cast<git_object **>(&mainCommit), mainRef, GIT_OBJ_COMMIT), 0);
	QCOMPARE(git_branch_create(&otherRef, repo, "other", mainCommit, 0), 0);
	git_reference_free(otherRef);
	git_commit_free(mainCommit);
	git_reference_free(mainRef);

	git_treebuilder *builder = nullptr;
	git_oid treeId;
	git_tree *tree = nullptr;
	git_signature *signature = nullptr;
	git_oid commitId;
	QCOMPARE(git_treebuilder_new(&builder, repo, nullptr), 0);
	QCOMPARE(git_treebuilder_write(&treeId, builder), 0);
	QCOMPARE(git_tree_lookup(&tree, repo, &treeId), 0);
	QCOMPARE(git_signature_now(&signature, "Subsurface test", "test@example.com"), 0);
	QCOMPARE(git_commit_create_v(&commitId, repo, "refs/heads/empty", signature, signature, nullptr,
				     "empty tree", tree, 0), 0);
	git_signature_free(signature);
	git_tree_free(tree);
	git_treebuilder_free(builder);
}

// AI-generated (Claude): Local bare remotes make replacement safety tests
// deterministic without depending on the shared cloud test service.
static bool pushMainBranch(git_repository *repo)
{
	git_remote *origin = nullptr;
	if (git_remote_lookup(&origin, repo, "origin"))
		return false;
	char refName[] = "refs/heads/main";
	char *ref = refName;
	git_strarray refspec { &ref, 1 };
	git_push_options opts = GIT_PUSH_OPTIONS_INIT;
	int result = git_remote_push(origin, &refspec, &opts);
	git_remote_free(origin);
	return result == 0;
}

static bool createReplacementRemote(const QString &root, std::string &remote, std::string &cache)
{
	std::string seed = QDir(root).filePath("seed").toStdString();
	remote = QDir(root).filePath("remote.git").toStdString();
	cache = QDir(root).filePath("cache").toStdString();
	if (!QDir().mkdir(QString::fromStdString(seed)))
		return false;

	git_repository *repo = nullptr;
	if (git_repository_init(&repo, seed.c_str(), false))
		return false;
	git_repository_free(repo);
	bool localOnly = git_local_only;
	git_local_only = true;
	int saveResult = save_dives((seed + "[main]").c_str());
	git_local_only = localOnly;
	if (saveResult)
		return false;
	if (git_repository_init(&repo, remote.c_str(), true))
		return false;
	git_repository_free(repo);
	if (git_repository_open(&repo, seed.c_str()))
		return false;
	git_remote *origin = nullptr;
	if (git_remote_create(&origin, repo, "origin", remote.c_str())) {
		git_repository_free(repo);
		return false;
	}
	git_remote_free(origin);
	bool pushed = pushMainBranch(repo);
	git_repository_free(repo);
	if (!pushed)
		return false;

	git_clone_options cloneOptions = GIT_CLONE_OPTIONS_INIT;
	cloneOptions.checkout_branch = "main";
	if (git_clone(&repo, remote.c_str(), cache.c_str(), &cloneOptions))
		return false;
	git_repository_free(repo);
	return true;
}

static void setReplacementInfo(git_info &info, const std::string &remote, const std::string &cache)
{
	info.url = remote;
	info.branch = "main";
	info.localdir = cache;
	info.transport = RT_OTHER;
}

static std::string branchHead(const std::string &repository)
{
	git_repository *repo = nullptr;
	if (git_repository_open(&repo, repository.c_str()))
		return std::string();
	std::string result = get_sha(repo, "main");
	git_repository_free(repo);
	return result;
}

static bool isDirectChild(const std::string &repository, const std::string &child, const std::string &parent)
{
	git_repository *repo = nullptr;
	git_oid childId;
	git_commit *commit = nullptr;
	bool result = !git_repository_open(&repo, repository.c_str()) &&
		!git_oid_fromstr(&childId, child.c_str()) &&
		!git_commit_lookup(&commit, repo, &childId) &&
		git_commit_parentcount(commit) == 1 &&
		!git_oid_strcmp(git_commit_parent_id(commit, 0), parent.c_str());
	git_commit_free(commit);
	git_repository_free(repo);
	return result;
}

static std::string concurrentRepository;
static std::string concurrentCommit;

static bool appendCommit(const std::string &repository, bool push, std::string &newCommit)
{
	git_repository *repo = nullptr;
	git_reference *head = nullptr;
	git_commit *parent = nullptr;
	git_tree *tree = nullptr;
	git_signature *signature = nullptr;
	git_oid commitId;
	bool ok = !git_repository_open(&repo, repository.c_str()) &&
		!git_branch_lookup(&head, repo, "main", GIT_BRANCH_LOCAL) &&
		!git_reference_peel(reinterpret_cast<git_object **>(&parent), head, GIT_OBJ_COMMIT) &&
		!git_commit_tree(&tree, parent) &&
		!git_signature_now(&signature, "Subsurface test", "test@example.com") &&
		!git_commit_create_v(&commitId, repo, "refs/heads/main", signature, signature, nullptr,
				     "test update", tree, 1, parent) && (!push || pushMainBranch(repo));
	if (ok) {
		char id[GIT_OID_HEXSZ + 1];
		git_oid_tostr(id, sizeof(id), &commitId);
		newCommit = id;
	}
	git_signature_free(signature);
	git_tree_free(tree);
	git_commit_free(parent);
	git_reference_free(head);
	git_repository_free(repo);
	return ok;
}

static int replacementProgress(const char *text)
{
	if (same_string(text, "Push confirmed replacement to remote storage") && concurrentCommit.empty() &&
	    !appendCommit(concurrentRepository, true, concurrentCommit))
		return -1;
	return 0;
}

void TestGitStorage::initTestCase()
{
	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences an proxy information
	prefs = default_prefs;
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
	qPrefProxy::load();
	qPrefCloudStorage::load();

	// setup our cloud test repo / credentials but allow the user to pick a different account by
	// setting these environment variables
	// Of course that email needs to exist as cloud storage account and have the given password
	//
	// To reduce the risk of collisions on the server, we have ten accounts set up for this purpose
	// please don't use them for other reasons as they will get deleted regularly
	email = qgetenv("SSRF_USER_EMAIL").toStdString();
	std::string password(qgetenv("SSRF_USER_PASSWORD").data());

	if (email.empty()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		email = format_string_std("gitstorage%d@hohndel.org", QRandomGenerator::global()->bounded(10));
#else
		// on Qt 5.9 we go back to using qsrand()/qrand()
		qsrand(time(NULL));
		email = format_string_std("gitstorage%d@hohndel.org", qrand() % 10);
#endif
	}
	if (password.empty())
		password = "please-only-use-this-in-the-git-tests";
	gitUrl = prefs.cloud_base_url;
	if (gitUrl.empty() || gitUrl.back() != '/')
		gitUrl += "/";
	gitUrl += "git";
	prefs.cloud_storage_email_encoded = email;
	prefs.cloud_storage_password = password.c_str();
	gitUrl += "/" + email;
	// all user storage for historical reasons always uses the user's email both as
	// repo name and as branch. To allow us to keep testing and not step on parallel
	// runs we'll use actually random branch names - yes, this still has a chance of
	// conflict, but I'm not going to implement a distributed lock manager for this
	if (starts_with(email, "gitstorage")) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		randomBranch = format_string_std("%x%x", QRandomGenerator::global()->bounded(0x1000000),
				QRandomGenerator::global()->bounded(0x1000000));
#else
		// on Qt 5.9 we go back to using qsrand()/qrand() -- if we get to this code, qsrand() was already called
		// even on a 32bit system RAND_MAX is at least 32767 so this will also give us 12 random hex digits
		randomBranch = format_string_std("%x%x%x%x", qrand() % 0x1000, qrand() % 0x1000,
				qrand() % 0x1000, qrand() % 0x1000);
#endif
	} else {
		// user supplied their own credentials, fall back to the usual "email is branch" pattern
		randomBranch = email;
	}
	cloudTestRepo = gitUrl + "[" + randomBranch + ']';
	localCacheDir = get_local_dir(gitUrl.c_str(), randomBranch.c_str());
	localCacheRepo = localCacheDir + "[" + randomBranch + "]";
	report_info("repo used: %s", cloudTestRepo.c_str());
	report_info("local cache: %s", localCacheRepo.c_str());

	// make sure we deal with any proxy settings that are needed
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(QString::fromStdString(prefs.proxy_host));
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(QString::fromStdString(prefs.proxy_user));
		proxy.setPassword(QString::fromStdString(prefs.proxy_pass));
	}
	QNetworkProxy::setApplicationProxy(proxy);

	// we will keep switching between online and offline mode below; let's always start online
	git_local_only = false;

	// initialize libgit2
	git_libgit2_init();

	// cleanup local and remote branches
	localRemoteCleanup();
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
}

void TestGitStorage::cleanupTestCase()
{
	localRemoteCleanup();
}

void TestGitStorage::cleanup()
{
	clear_dive_file_data();
}

void TestGitStorage::testGitStorageLocal_data()
{
	// Test different paths we may encounter (since storage depends on user name)
	// as well as with and without "file://" URL prefix.
	using namespace std::string_literals; // For std::string literals: "some string"s.
	QTest::addColumn<std::string>("testDirName");
	QTest::addColumn<std::string>("prefixRead");
	QTest::addColumn<std::string>("prefixWrite");
	QTest::newRow("ASCII path") << "./gittest"s << ""s << ""s;
	QTest::newRow("Non ASCII path") << "./gittest_éèêôàüäößíñóúäåöø"s << ""s << ""s;
	QTest::newRow("ASCII path with file:// prefix on read") << "./gittest2"s << "file://"s << ""s;
	QTest::newRow("Non ASCII path with file:// prefix on read") << "./gittest2_éèêôàüäößíñóúäåöø"s << ""s << "file://"s;
	QTest::newRow("ASCII path with file:// prefix on write") << "./gittest3"s << "file://"s << ""s;
	QTest::newRow("Non ASCII path with file:// prefix on write") << "./gittest3_éèêôàüäößíñóúäåöø"s << ""s << "file://"s;
}

void TestGitStorage::testGitStorageLocal()
{
	// test writing and reading back from local git storage
	git_repository *repo;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	QFETCH(std::string, testDirName);
	QFETCH(std::string, prefixRead);
	QFETCH(std::string, prefixWrite);
	QDir testDir(testDirName.c_str());
	QCOMPARE(testDir.removeRecursively(), true);
	QCOMPARE(QDir().mkdir(testDirName.c_str()), true);
	std::string repoNameRead = prefixRead + testDirName;
	std::string repoNameWrite = prefixWrite + testDirName;
	QCOMPARE(git_repository_init(&repo, testDirName.c_str(), false), 0);
	QCOMPARE(save_dives((repoNameWrite + "[test]").c_str()), 0);
	QCOMPARE(save_dives("./SampleDivesV3.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file((repoNameRead + "[test]").c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesV3viagit.ssrf"), 0);
	QFile org("./SampleDivesV3.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3viagit.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
}

// AI-generated (Claude)
void TestGitStorage::testGitSavePreflight()
{
	QTemporaryDir destinationDir;
	QTemporaryDir sourceDir;
	QVERIFY(destinationDir.isValid());
	QVERIFY(sourceDir.isValid());
	std::string destination = destinationDir.path().toStdString();
	std::string source = sourceDir.path().toStdString();
	std::string mainTarget = destination + "[main]";
	std::string otherTarget = destination + "[other]";
	std::string newTarget = destination + "[new]";
	std::string emptyTarget = destination + "[empty]";
	std::string sourceTarget = source + "[source]";
	std::string xmlCopy = destinationDir.filePath("loaded.ssrf").toStdString();

	git_repository *repo = nullptr;
	QCOMPARE(git_repository_init(&repo, destination.c_str(), false), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	QCOMPARE(save_dives(mainTarget.c_str()), 0);
	createTestBranches(repo);
	git_repository_free(repo);

	clear_dive_file_data();
	QCOMPARE(parse_file(mainTarget.c_str(), &divelog), 0);
	QVERIFY(!loaded_git_provenance.empty());
	std::string loadedRepository = loaded_git_provenance.repository;
	std::string loadedBranch = loaded_git_provenance.branch;
	std::string loadedCommit = loaded_git_provenance.commit;
	QVERIFY(savePreflight(mainTarget).status == git_save_preflight_status::allowed);
	QVERIFY(savePreflight(otherTarget).status == git_save_preflight_status::replacement_confirmation_required);
	QVERIFY(savePreflight(newTarget).status == git_save_preflight_status::allowed);
	QVERIFY(savePreflight(emptyTarget).status == git_save_preflight_status::allowed);

	QCOMPARE(save_dives(xmlCopy.c_str()), 0);
	QCOMPARE(loaded_git_provenance.repository, loadedRepository);
	QCOMPARE(loaded_git_provenance.branch, loadedBranch);
	QCOMPARE(loaded_git_provenance.commit, loadedCommit);
	QVERIFY(savePreflight(mainTarget).status == git_save_preflight_status::allowed);

	clear_dive_file_data();
	QVERIFY(loaded_git_provenance.empty());
	QCOMPARE(parse_file(xmlCopy.c_str(), &divelog), 0);
	QVERIFY(loaded_git_provenance.empty());
	QVERIFY(savePreflight(mainTarget).status == git_save_preflight_status::replacement_confirmation_required);
	std::string oldHead;
	{
		git_info info;
		QVERIFY(is_git_repository(mainTarget.c_str(), &info));
		QVERIFY(open_git_repository(&info));
		oldHead = get_sha(info.repo, info.branch);
	}
	QVERIFY(save_dives(mainTarget.c_str()) != 0);
	{
		git_info info;
		QVERIFY(is_git_repository(mainTarget.c_str(), &info));
		QVERIFY(open_git_repository(&info));
		QCOMPARE(get_sha(info.repo, info.branch), oldHead);
	}

	clear_dive_file_data();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	QCOMPARE(git_repository_init(&repo, source.c_str(), false), 0);
	git_repository_free(repo);
	QCOMPARE(save_dives(sourceTarget.c_str()), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(sourceTarget.c_str(), &divelog), 0);
	QVERIFY(savePreflight(mainTarget).status == git_save_preflight_status::replacement_confirmation_required);

	std::string invalidTarget = destinationDir.filePath("missing").toStdString() + "[main]";
	QVERIFY(savePreflight(invalidTarget).status == git_save_preflight_status::destination_missing);
	QVERIFY(savePreflight(destination + "[invalid branch name]").status == git_save_preflight_status::error);

	QTemporaryDir deletedCacheDir;
	QVERIFY(deletedCacheDir.isValid());
	std::string deletedCache = deletedCacheDir.path().toStdString();
	std::string deletedCacheTarget = deletedCache + "[main]";
	QCOMPARE(git_repository_init(&repo, deletedCache.c_str(), false), 0);
	git_repository_free(repo);
	QCOMPARE(save_dives(deletedCacheTarget.c_str()), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(deletedCacheTarget.c_str(), &divelog), 0);
	QVERIFY(QDir(QString::fromStdString(deletedCache)).removeRecursively());
	QVERIFY(savePreflight(deletedCacheTarget).status == git_save_preflight_status::allowed);

	QTemporaryDir remoteFixture;
	QVERIFY(remoteFixture.isValid());
	clear_dive_file_data();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &divelog), 0);
	std::string remote;
	std::string cache;
	QVERIFY(createReplacementRemote(remoteFixture.path(), remote, cache));
	std::string remoteHead = branchHead(remote);
	QVERIFY(QDir(QString::fromStdString(cache)).removeRecursively());
	clear_dive_file_data();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	git_info missingCacheInfo;
	setReplacementInfo(missingCacheInfo, remote, cache);
	QVERIFY(preflight_git_save(&missingCacheInfo).status == git_save_preflight_status::destination_missing);
	QVERIFY(git_save_dives(&missingCacheInfo, false) != 0);
	QCOMPARE(branchHead(remote), remoteHead);
}

// AI-generated (Claude): Empty sites referenced by dives must survive git
// serialization, while genuinely unused empty sites remain omitted.
void TestGitStorage::testGitStorageReferencedEmptyDiveSite()
{
	QTemporaryDir destinationDir;
	QVERIFY(destinationDir.isValid());
	std::string destination = destinationDir.path().toStdString();
	std::string target = destination + "[main]";
	git_repository *repo = nullptr;
	QCOMPARE(git_repository_init(&repo, destination.c_str(), false), 0);
	git_repository_free(repo);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &divelog), 0);
	QVERIFY(!divelog.dives.empty());

	dive_site *referenced = divelog.dives.front()->dive_site;
	QVERIFY(referenced);
	referenced->name.clear();
	referenced->description.clear();
	referenced->notes.clear();
	referenced->location = {};
	referenced->taxonomy.clear();
	uint32_t referencedUuid = referenced->uuid;
	dive_site *unreferenced = divelog.sites.create(std::string());
	uint32_t unreferencedUuid = unreferenced->uuid;
	QCOMPARE(save_dives(target.c_str()), 0);

	clear_dive_file_data();
	QCOMPARE(parse_file(target.c_str(), &divelog), 0);
	QVERIFY(!divelog.dives.empty());
	QVERIFY(divelog.dives.front()->dive_site);
	QCOMPARE(divelog.dives.front()->dive_site->uuid, referencedUuid);
	bool foundUnreferenced = false;
	for (const auto &site: divelog.sites)
		foundUnreferenced |= site->uuid == unreferencedUuid;
	QVERIFY(!foundUnreferenced);
}

// AI-generated (Claude): Verify a complete replacement writes the current log,
// retains the prior remote tip as its direct parent, and preflight never pushes.
void TestGitStorage::testGitStorageReplacement()
{
	QTemporaryDir fixture;
	QVERIFY(fixture.isValid());
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &divelog), 0);
	divelog.dives.front()->notes = "obsolete remote dive";
	std::string remote;
	std::string cache;
	QVERIFY(createReplacementRemote(fixture.path(), remote, cache));
	std::string oldRemoteHead = branchHead(remote);
	QVERIFY(!oldRemoteHead.empty());
	std::string unpublishedCacheHead;
	QVERIFY(appendCommit(cache, false, unpublishedCacheHead));
	QVERIFY(unpublishedCacheHead != oldRemoteHead);

	clear_dive_file_data();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	const size_t expectedDives = divelog.dives.size();
	git_info info;
	setReplacementInfo(info, remote, cache);
	QVERIFY(preflight_git_save(&info).status == git_save_preflight_status::replacement_confirmation_required);
	QCOMPARE(branchHead(remote), oldRemoteHead);
	QCOMPARE(git_replace_dives(&info), 0);

	std::string replacementHead = branchHead(remote);
	QVERIFY(replacementHead != oldRemoteHead);
	QVERIFY(replacementHead != unpublishedCacheHead);
	QVERIFY(isDirectChild(remote, replacementHead, oldRemoteHead));
	clear_dive_file_data();
	QCOMPARE(parse_file((cache + "[main]").c_str(), &divelog), 0);
	QCOMPARE(divelog.dives.size(), expectedDives);
	for (const auto &dive: divelog.dives)
		QVERIFY(dive->notes != "obsolete remote dive");
}

// AI-generated (Claude): A confirmed snapshot is complete even when the
// current log contains only part of the destination's former dive set.
void TestGitStorage::testGitStoragePartialReplacement()
{
	QTemporaryDir fixture;
	QVERIFY(fixture.isValid());
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	std::string remote;
	std::string cache;
	QVERIFY(createReplacementRemote(fixture.path(), remote, cache));

	clear_dive_file_data();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	QVERIFY(divelog.dives.size() > 1);
	while (divelog.dives.size() > 1)
		divelog.delete_multiple_dives(std::vector<dive *>{ divelog.dives.back().get() });
	const timestamp_t retainedDive = divelog.dives.front()->when;
	git_info info;
	setReplacementInfo(info, remote, cache);
	QCOMPARE(git_replace_dives(&info), 0);

	clear_dive_file_data();
	QCOMPARE(parse_file((cache + "[main]").c_str(), &divelog), 0);
	QCOMPARE(divelog.dives.size(), size_t(1));
	QCOMPARE(divelog.dives.front()->when, retainedDive);
}

// AI-generated (Claude): Advance the bare remote after replacement refresh but
// before its push, proving the non-forced push rejects the stale confirmation.
void TestGitStorage::testGitStorageReplacementConcurrentUpdate()
{
	QTemporaryDir fixture;
	QVERIFY(fixture.isValid());
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	std::string remote;
	std::string cache;
	QVERIFY(createReplacementRemote(fixture.path(), remote, cache));
	concurrentRepository = QDir(fixture.path()).filePath("concurrent").toStdString();
	git_repository *repo = nullptr;
	git_clone_options cloneOptions = GIT_CLONE_OPTIONS_INIT;
	cloneOptions.checkout_branch = "main";
	QCOMPARE(git_clone(&repo, remote.c_str(), concurrentRepository.c_str(), &cloneOptions), 0);
	git_repository_free(repo);
	const std::string cacheHeadBefore = branchHead(cache);

	clear_dive_file_data();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &divelog), 0);
	const size_t expectedDives = divelog.dives.size();
	const timestamp_t expectedWhen = divelog.dives.front()->when;
	const std::string provenanceBefore = loaded_git_provenance.commit;
	concurrentCommit.clear();
	set_git_update_cb(&replacementProgress);
	git_info info;
	setReplacementInfo(info, remote, cache);
	int replacementResult = git_replace_dives(&info);
	set_git_update_cb(nullptr);
	QVERIFY(replacementResult != 0);

	QVERIFY(!concurrentCommit.empty());
	QCOMPARE(branchHead(remote), concurrentCommit);
	QCOMPARE(branchHead(cache), cacheHeadBefore);
	QCOMPARE(divelog.dives.size(), expectedDives);
	QCOMPARE(divelog.dives.front()->when, expectedWhen);
	QCOMPARE(loaded_git_provenance.commit, provenanceBefore);
	concurrentRepository.clear();
	concurrentCommit.clear();
}

void TestGitStorage::testGitStorageCloud()
{
	// test writing and reading back from cloud storage
	// connect to the ssrftest repository on the cloud server
	// and repeat the same test as before with the local git storage
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &divelog), 0);
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesV3viacloud.ssrf"), 0);
	QVERIFY(savePreflight(cloudTestRepo).status == git_save_preflight_status::allowed);
	QFile org("./SampleDivesV3.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3viacloud.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudOfflineSync()
{
	// make a change to local cache repo (pretending that we did some offline changes)
	// and then open the remote one again and check that things were propagated correctly
	// read the local repo from the previous test and add dive 10
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QVERIFY(savePreflight(localCacheRepo).status == git_save_preflight_status::allowed);
	std::string loadedCommit = loaded_git_provenance.commit;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &divelog), 0);
	// calling process_loaded_dives() sorts the table, but calling add_imported_dives()
	// causes it to try to update the window title... let's not do that
	divelog.process_loaded_dives();
	// now save only to the local cache but not to the remote server
	git_local_only = true;
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	QVERIFY(loaded_git_provenance.commit != loadedCommit);
	QVERIFY(savePreflight(cloudTestRepo).status == git_save_preflight_status::allowed);
	// AI-generated (Claude): Compare serialized offline and online git states.
	clear_dive_file_data();
	QCOMPARE(parse_file(localCacheRepo.c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10local.ssrf"), 0);
	clear_dive_file_data();
	// now pretend that we are online again and open the cloud storage and compare
	git_local_only = false;
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10viacloud.ssrf"), 0);
	QFile org("./SampleDivesV3plus10local.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3plus10viacloud.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	// write back out to cloud storage, move away the local cache, open again and compare
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();
	moveDir(localCacheDir, localCacheDir + "save");
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10fromcloud.ssrf"), 0);
	org.close();
	org.open(QFile::ReadOnly);
	QFile out2("./SampleDivesV3plus10fromcloud.ssrf");
	out2.open(QFile::ReadOnly);
	QTextStream orgS2(&org);
	QTextStream outS2(&out2);
	readin = orgS2.readAll();
	written = outS2.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge()
{
	// we want to test a merge - in order to do so we need to make changes to the cloud
	// repo from two clients - but since we have only one client here, we have to cheat
	// a little:
	// the local cache with the 'save' extension will serve as our second client;
	//
	// (1) first we make a change and save it to the cloud
	// (2) then we switch to the second client (i.e., we move that cache back in place)
	// (3) on that second client we make a different change while offline
	// (4) now we take that second client back online and get the merge
	// (5) let's make sure that we have the expected data on the second client
	// (6) go back to the first client and ensure we have the same data there after sync

	// (1) open the repo, add dive test11 and save to the cloud
	git_local_only = false;
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test11.xml", &divelog), 0);
	divelog.process_loaded_dives();
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();

	// (2) switch to the second client by moving the old cache back in place
	moveDir(localCacheDir, localCacheDir + "client1");
	moveDir(localCacheDir + "save", localCacheDir);

	// (3) open the repo from the old cache and add dive test12 while offline
	git_local_only = true;
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test12.xml", &divelog), 0);
	divelog.process_loaded_dives();
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();

	// (4) now take things back online
	git_local_only = false;
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	clear_dive_file_data();

	// (5) now we should have all the dives in our repo on the second client
	// first create the reference data from the xml files:
	QCOMPARE(parse_file("./SampleDivesV3plus10local.ssrf", &divelog), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test11.xml", &divelog), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test12.xml", &divelog), 0);
	divelog.process_loaded_dives();
	// AI-generated (Claude): Normalize calculated fields through git before comparing git states.
	QTemporaryDir referenceDir;
	QVERIFY(referenceDir.isValid());
	std::string referenceRepository = referenceDir.path().toStdString();
	std::string referenceTarget = referenceRepository + "[reference]";
	git_repository *referenceRepo = nullptr;
	QCOMPARE(git_repository_init(&referenceRepo, referenceRepository.c_str(), false), 0);
	git_repository_free(referenceRepo);
	QCOMPARE(save_dives(referenceTarget.c_str()), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(referenceTarget.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12.ssrf"), 0);
	// then load from the cloud
	clear_dive_file_data();
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12-merged.ssrf"), 0);
	// finally compare what we have
	QFile org("./SampleDivesV3plus10-11-12-merged.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3plus10-11-12.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();

	// (6) move ourselves back to the first client and compare data there
	moveDir(localCacheDir + "client1", localCacheDir);
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12-merged-client1.ssrf"), 0);
	QFile client1("./SampleDivesV3plus10-11-12-merged-client1.ssrf");
	client1.open(QFile::ReadOnly);
	QTextStream client1S(&client1);
	readin = client1S.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge2()
{
	// delete a dive offline
	// edit the same dive in the cloud repo
	// merge
	// (1) open repo, delete second dive, save offline
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QVERIFY(divelog.dives.size() >= 2);
	divelog.delete_multiple_dives(std::vector<struct dive *>{ divelog.dives[1].get() });
	QCOMPARE(save_dives("./SampleDivesMinus1.ssrf"), 0);
	git_local_only = true;
	QCOMPARE(save_dives(localCacheRepo.c_str()), 0);
	git_local_only = false;
	clear_dive_file_data();

	// (2) move cache out of the way
	moveDir(localCacheDir, localCacheDir + "save");

	// (3) now we open the cloud storage repo and modify that second dive
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QVERIFY(divelog.dives.size() >= 2);
	divelog.process_loaded_dives();
	divelog.dives[1]->notes = "These notes have been modified by TestGitStorage";
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();

	// (4) move the saved local cache  backinto place and try to open the cloud repo
	//     -> this forces a merge
	moveDir(localCacheDir + "save", localCacheDir);
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesMinus1-merged.ssrf"), 0);
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	QFile org("./SampleDivesMinus1-merged.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesMinus1.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge3()
{
	// create multi line notes and store them to the cloud repo and local cache
	// edit dive notes offline
	// edit the same dive notes in the cloud repo
	// merge


	// (1) open repo, edit notes of first three dives
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QVERIFY(divelog.dives.size() >= 3);
	divelog.dives[0]->notes = "Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough";
	divelog.dives[1]->notes = "Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough";
	divelog.dives[2]->notes = "Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough";
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();

	// (2) make different edits offline
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QVERIFY(divelog.dives.size() >= 3);
	divelog.dives[0]->notes = "Create multi line dive notes\nDifferent line 2 and removed 3-5\n\nThat should be enough";
	divelog.dives[1]->notes = "Line 2\nLine 3\nLine 4\nLine 5"; // keep the middle, remove first and last");
	divelog.dives[2]->notes = "single line dive notes";
	git_local_only = true;
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	git_local_only = false;
	clear_dive_file_data();

	// (3) simulate a second system by moving the cache away and open the cloud storage repo and modify
	//     those first dive notes differently while online
	moveDir(localCacheDir, localCacheDir + "save");
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	divelog.process_loaded_dives();
	QVERIFY(divelog.dives.size() >= 3);
	divelog.dives[0]->notes = "Completely different dive notes\nBut also multi line";
	divelog.dives[1]->notes = "single line dive notes";
	divelog.dives[2]->notes = "Line 2\nLine 3\nLine 4\nLine 5"; // keep the middle, remove first and last");
	QCOMPARE(save_dives(cloudTestRepo.c_str()), 0);
	clear_dive_file_data();

	// (4) move the saved local cache back into place and open the cloud repo -> this forces a merge
	moveDir(localCacheDir + "save", localCacheDir);
	QCOMPARE(parse_file(cloudTestRepo.c_str(), &divelog), 0);
	QCOMPARE(save_dives("./SampleDivesMerge3.ssrf"), 0);
	// we are not trying to compare this to a pre-determined result... what this test
	// checks is that there are no parsing errors with the merge
}

QTEST_GUILESS_MAIN(TestGitStorage)
