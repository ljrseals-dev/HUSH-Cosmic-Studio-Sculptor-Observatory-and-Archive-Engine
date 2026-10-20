#include "hush/project_close_policy.hpp"

#include <cassert>

int main() {
    using hush::ProjectCloseChoice;
    using hush::should_close_project;

    assert(should_close_project(false, ProjectCloseChoice::Cancel, false));
    assert(should_close_project(false, ProjectCloseChoice::Save, false));
    assert(should_close_project(true, ProjectCloseChoice::Discard, false));
    assert(should_close_project(true, ProjectCloseChoice::Save, true));
    assert(!should_close_project(true, ProjectCloseChoice::Save, false));
    assert(!should_close_project(true, ProjectCloseChoice::Cancel, false));
    return 0;
}
