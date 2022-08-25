set -eu

if [ $# -ne 4 ]; then
    echo >&2 "Error: wrong number of arguments"
    echo
    echo "Usage:  add-file-to-store.sh NAME VERSION FILE POTIONDIR"
    exit 105
fi

[ -n "$4" ] && [ "$4" != / ]
potion_dir="$4"

[ -n "$3" ] && [ "$3" != / ]

make bin/base24 bin/nar bin/sha3-256sum

t="$(mktemp -d)"
[ -d "$t" ]
[ "$t" != / ]
rmtemp() {
    e=$?
    echo "Removing temp dir"
    rm -rf "$t"
    trap - INT TERM EXIT
    exit $e
}
trap rmtemp INT TERM EXIT
echo "Using temp dir $t"

cp "$3" "$t"
dir_digest="$(bin/nar "$t" | bin/sha3-256sum -b | bin/base24)"
echo "Dir digest is $dir_digest"
rm "$t/$(basename "$3")"
cat >"$t/rec" <<END
n $1
v $2
d $dir_digest
END
recipe_path="$(bin/nar "$t/rec" | bin/sha3-256sum -b | bin/base24 -f)"
echo "Recipe path is $recipe_path"
mkdir -p "$(dirname "$potion_dir/recipe/$recipe_path")"
cp -i "$t/rec" "$potion_dir/recipe/$recipe_path"
mkdir -p "$potion_dir/$recipe_path"
cp -i "$3" "$potion_dir/$recipe_path"
rm -r "$t"
