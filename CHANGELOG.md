# Changelog

Notable changes to Scenery Editor X, newest first. It starts after 0.2.63.564; earlier changes are in the pull request
descriptions and the git history. Versions are `SEDX_VERSION_STRING` in `source/resource.h`.

## Unreleased

### Removed

- `Ref(const std::shared_ptr<T>&)` in `utils/pointers.h`, removed in 0.2.52.553 (merged to `master` with PR #65). It shared
  the object with the `shared_ptr` without the `shared_ptr` knowing: unless the `shared_ptr` came from
  `Ref::ToSharedPtr()`, each side deleted the object on its own last release. Keep the object in a `Ref`
  (`CreateRef<T>(...)`) and call `ToSharedPtr()` where an API wants a `std::shared_ptr`. (#66)
