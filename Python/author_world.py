"""Builds MobWorld's content shell.

    World -> Rebuild Content

or by hand:

    import author_world
    author_world.build_all()

There is almost nothing here on purpose. MobWorld's job is joining plugins that already draw
things, and the sky projection is one of them: the engine's HDRI Backdrop plugin ships the
materials that put a cubemap on a dome and project it onto the ground, and reimplementing those
would be a second copy of somebody else's shader to keep in step.

So what MobWorld owns is an instance of each. That gives a project something to point at, and
somewhere to put an override, without anybody editing engine plugin content in place.

Two of them, because the dome has two material slots. The sky half and the ground half are
different shaders and putting one in both slots draws the ground projection across the sky.

Everything lands under a subfolder. A plugin that drops assets in its content root makes every
project that installs it untidy for as long as it is installed.
"""

import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

ROOT = '/MobWorld/Sky'

# The engine's HDRI Backdrop plugin. AMobWorldSky writes the parameter names these declare.
INSTANCES = [
    (ROOT + '/MI_MobWorldSky', '/HDRIBackdrop/Materials/HDRI_Projection_Sky.HDRI_Projection_Sky'),
    (ROOT + '/MI_MobWorldFloor', '/HDRIBackdrop/Materials/HDRI_Projection_Floor.HDRI_Projection_Floor'),
]


def _log(msg):
    unreal.log('[MobWorld] ' + str(msg))


def _tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def build_instance(path, master_path):
    """One instance over one of the engine's projection masters."""
    master = unreal.load_asset(master_path)
    if master is None:
        unreal.log_warning('[MobWorld] %s is missing. Is the HDRI Backdrop plugin enabled?'
                           % master_path)
        return None

    if EAL.does_asset_exist(path):
        instance = unreal.load_asset(path)
    else:
        package, _, name = path.rpartition('/')
        instance = _tools().create_asset(name, package, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())

    MEL.set_material_instance_parent(instance, master)
    EAL.save_loaded_asset(instance, only_if_is_dirty=False)

    _log('built ' + path)
    return instance


def build_all():
    """Everything MobWorld owns, which is two material instances."""
    return all(build_instance(path, master) is not None for path, master in INSTANCES)
