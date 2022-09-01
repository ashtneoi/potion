set -eu

if [ $# -ne 4 ]; then
    echo >&2 "Error: wrong number of arguments"
    echo
    echo "Usage:  add-file-to-store.sh NAME VERSION FILE POTIONDIR"
    exit 105
fi

name="$1"
version="$2"
file="$3"
potion_dir="$4"

t="$(mktemp -d)"
[ -d "$t" ] && [ "$t" != / ]
rmtemp() {
    e=$?
    echo "Removing temp dir"
    rm -rf "$t"
    trap - INT TERM EXIT
    exit $e
}
trap rmtemp INT TERM EXIT
echo "Using temp dir $t"

cp -Pr -- "$file" "$t/"
dir_digest="$(bin/nar "$t" | bin/sha3-256sum -b | bin/base24)"
echo "Dir digest is $dir_digest"
rm -r -- "$t/$(basename "$file")"

cat >"$t/rec" <<END
n $name
v $version
o - - -
END
recipe_path="$(bin/nar "$t/rec" | bin/sha3-256sum -b | bin/base24 -p)"
echo "Recipe path is $recipe_path"
mkdir -p "$(dirname "$potion_dir/_/$recipe_path")"

cat >"$potion_dir/_/$recipe_path" <<END
n $name
v $version
o - $recipe_path $dir_digest
END

mkdir -p "$potion_dir/$recipe_path"
cp -Pr "$file" "$potion_dir/$recipe_path"

# The EXIT trap handler will remove $t for us.
