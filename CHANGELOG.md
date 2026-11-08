# Changelog

Notable changes to Scenery Editor X, newest first. It starts after 0.2.63.564; earlier changes are in the pull request
descriptions and the git history. Versions are `SEDX_VERSION_STRING` in `source/resource.h`.

## Unreleased

### Removed

- `Ref(const std::shared_ptr<T>&)` in `utils/pointers.h`, removed in 0.2.52.553 (merged to `master` with PR #65). It shared
  the object with the `shared_ptr` without the `shared_ptr` knowing: unless the `shared_ptr` came from
  `Ref::ToSharedPtr()`, each side deleted the object on its own last release. Keep the object in a `Ref`
  (`CreateRef<T>(...)`) and call `ToSharedPtr()` where an API wants a `std::shared_ptr`. (#66)

### Fixed

- A `WeakRef` to an object destroyed without its last `Ref` release (deleted by hand, or a stack or member object a `Ref`
  pointed to) now expires. Before, the object's control block stayed registered under its old address, and the next
  object there inherited it: that object's new WeakRefs were expired at once, or the old ones locked it. Both cases are
  misuse; `~RefCounted()` now cleans up after them. (#67, 0.2.64.565)
