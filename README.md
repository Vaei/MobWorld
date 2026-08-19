# Mob World <img align="right" width=128, height=128 src="https://github.com/Vaei/MobWorld/blob/main/Resources/Icon128.png">

> [!IMPORTANT]
> Joins the Mob rendering plugins together

Connects the Mob rendering plugins together. Which sky, how it reflects, what the weather is - pushed into every Mob plugin, along with additional functionality.

Requires **MobFort** and **MobWater**, whose types it uses. MobLights and MobMaterials are reached by soft path to their collections, so dropping those needs no code change.

UE5.8+

## Setting up

**World → Set Up This Project.** Makes a sky set, points the settings at it, builds the material the backdrop draws with, and puts a backdrop in the open level. Safe to run more than once.

Then you will need to:

1. **Fill the sky set.** A cubemap per sky, and a panorama beside it from **World → Panorama From Cubemap** with the cubemaps selected. Put the `MaxMip` the Output Log reports next to the panorama.
2. **Use `UMobWorldSunComponent` for the level's directional light**, in place of a plain one. It is the light, not a component beside it.

## What it joins

| Plugin | Wants | Gets it from | Fails as |
|---|---|---|---|
| MobFort | `MPC_FortLighting.SunDirection` / `.SunColor` | `UMobWorldSunComponent` | Characters lit from wherever the collection was last saved |
| MobFort | `MPC_FortLighting.SkyYaw` | `AMobWorldSky`, from the sky it is showing | The sun in one place on screen and another in every reflection |
| MobFort | `SpecPanorama`, `Atlas`, `SpecularScalar` | `UMobWorldSubsystem`, on registered instances | Flat grey characters |
| MobFort | Custom primitive data 6 and 7 | `UMobWorldWetnessComponent` | Nobody ever gets wet |
| MobFort | Custom primitive data 8 | `UMobWorldAreaLightComponent` | A cellar as bright as the courtyard |
| MobMaterials | `MPC_MobWeather.Wetness` / `.Snow` / `.PuddleAmount` | `UMobWorldSubsystem::SetWeather` | World surfaces never get wet |

## Pieces

**`FMobWorldSkyEntry`** - one sky, and everything that changes with it: cubemap, panorama, yaw, intensity, gradient atlases, specular, max mip, and the backdrop's size and projection settings. A struct rather than parallel lists because they are one answer; a cube swapped without its panorama renders happily and wrongly.

The assets are soft. A set holds every sky a game has, and hard pointers would mean opening it pulled every HDRI, panorama and atlas into memory at once.

**`UMobWorldSkySet`** - the list. Order is important.

**`UMobWorldSubsystem`** - holds the current sky and applies it. Streams the standing sky through the asset manager, releasing the previous handle so walking through skies does not accumulate them. `PreloadSky` for a change you can see coming to avoid the async load delay.

Keeps a weak list of every material instance it has fed and rewrites them all on a sky change, so an owner that never binds a delegate still follows. Left to each owner to remember.

**`AMobWorldSky`** - the backdrop: dome, ground projection, sky light. Almost nothing is set on the actor, because which sky it shows and how it looks belong to the sky set. Where you put it is the projection centre.

**`UMobWorldSunComponent`** - a directional light that writes itself into MobFort's collection on register, on movement and on any render state change. No tick.

**`UMobWorldWetnessComponent`** - MobFort's wetness answered by MobWater. One function, and the original reason this plugin exists.

**`UMobWorldAreaLightComponent`** and **`AMobWorldLightVolume`** - a part of the level that is darker than the level as a whole, per character. Box, sphere or capsule.

## Gradients per character

`FMobWorldSkyEntry::GradientAtlases` is keyed by `FGameplayTag`, because what decides a character's gradients differs per project: a role for one, an outfit out of an inventory for another. A project that wants none of it leaves the single empty-tag entry alone and never passes a tag. An unlisted tag falls back to the empty one, so adding a sky never means re-listing every character.

## Things that fail silently

Every one of these renders. None of them logs anything.

| Symptom | Cause |
|---|---|
| Characters lit from the wrong direction | Nothing writes `MPC_FortLighting`. The masters are Unlit, so no light ever reaches them |
| Right in the editor, wrong in PIE and in a build | ForwardRender's tick is editor-only. That is what the sun component is for |
| Every character blows out when the level brightens | A light actor's intensity copied into `SunColor.a`. It is lux, and an unlit master has no exposure |
| Flat grey characters | `SpecPanorama` still on the placeholder |
| Every surface a mirror at every roughness | The panorama has no mip chain |
| Reflections yawed by a constant angle | The backdrop and the panorama disagree. They rotate the same sky from opposite sides |
| A prop tinted by how wet its owner is | Custom primitive data slots overlapping between plugins |
| Wetness does nothing whatever is written | `bWetness` not ticked on the instance |
| A volume that darkens nobody | It contains nothing |
