# Shared Creep Colony service: staged profiles

Creep-to-Sunken/Spore payment and in-place admission are shared by campaign
and custom-game paths. Custom games retain their existing target-raw encoding;
Z7 and Z8 retain the compact campaign encoding, with raw Creep identity until
completion and a reserved morph flag and HP snapshot. No save format is
silently converted. The campaign load validator checks the marker, owner,
source identity, construction clock, and HP snapshot before restoring it.

Z8 human slot 3 permissions for Creep, Sunken, Spore, Hatchery, Pool and
Evolution Chamber were resolved from the authenticated scenario PUNI table.
The generated bitmap remains a private build input; the checked-in generator
and bounded accessor reject invalid owners and IDs. The ordinary Z8 worker
build and the morph target consult that bitmap. Morphs also require a completed
Creep Colony and a completed Pool (Sunken) or Evolution Chamber (Spore), and
charge the owning bank. The Z8 sprite bank and construction table contain
entries for raw 143, 144 and 146. Z8's existing completed-Spore detection
service remains separate from the construction service.

Enabling the shared campaign command in other missions requires that profile's
source permissions, actor/art mappings, candidate-save validation, and memory
reserve be qualified. This slice does not infer those facts from Z8 or enable
other mission predicates. The custom-game implementation retains its existing
source-independent catalog rules and save representation. This is not a claim
of complete native construction, collision, detection, or AI equivalence.

`make test-sc64-shared-colony` uses prepared private profile inputs. The Z8
fixture follows ordinary menu, worker arrival, construction, morph and valid
mid-morph pause-save/load, then advances normal updates through a delayed
Spore impact. A queued lethal impact also verifies that immediate pause-save
and load after morphing-colony destruction retain no transient marker. It seeds an Evolution Chamber when none is already present and
is not a full organic technology progression. Its canonical and derived-save
variants cover their respective live save paths. The typed compact fixture
checks marker and HP-snapshot codec preservation independently. The Z7 impact
fixture advances actual game updates; the skirmish fixture checks both owner
banks and the existing in-flight representation.
