# motorIms Releases

## __R1-0-2 (2023-04-11)__
R1-0-2 is a release based on the master branch.

### Changes since R1-0-1

#### New features
* None

#### Modifications to existing features
* None

#### Bug fixes
* None

#### Continuous integration
* Added ci-scripts (v3.0.1)
* Configured to use Github Actions for CI

## __R1-0-1 (2020-05-11)__
R1-0-1 is a release based on the master branch.  

### Changes since R1-0

#### New features
* None

#### Modifications to existing features
* None

#### Bug fixes
* Commit [95ead31](https://github.com/epics-motor/motorIms/commit/95ead31379acbd03e729eb5124a278f292e7395c): Include ``$(MOTOR)/modules/RELEASE.$(EPICS_HOST_ARCH).local`` instead of ``$(MOTOR)/configure/RELEASE``
* Commit [10c502c](https://github.com/epics-motor/motorIms/commit/10c502c8dc118591f63cc4c9cdc41127883ff1f5): Eliminated compiler warnings

## __R1-0 (2019-04-18)__
R1-0 is a release based on the master branch.  

### Changes since motor-6-11

motorIms is now a standalone module, as well as a submodule of [motor](https://github.com/epics-modules/motor)

#### New features
* motorIms can be built outside of the motor directory
* motorIms has a dedicated example IOC that can be built outside of motorIms

#### Modifications to existing features
* None

#### Bug fixes
* None
