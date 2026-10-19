# HUSH recovery process-restart test

The `hush_recovery_restart_tests` CTest launches itself as a child process. The child appends a complete project snapshot through `RecoveryJournal::append_snapshot` and exits immediately with `std::_Exit`, avoiding normal C++ teardown. The parent then opens the same journal and checks that the snapshot and track identity recover correctly.

This tests a completed journal write across process exit and a fresh recovery-reader context. It complements the torn-tail unit test, but it is **not** a power-loss test, does not simulate termination in the middle of the filesystem write, and does not establish directory-entry durability or guarantees for every filesystem/storage controller. Those remain target-platform validation tasks.
