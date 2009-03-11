#!/bin/zsh

# makes all lib-linking to relative paths
# code under GPL
# by Albert Zeyer
# some more code by Imendio AB

fix_library_prefixes()
{
  directory=$1
  old_prefix=$2
  new_prefix=$3
 
  [ ! -d $directory ] && return
  pushd . > /dev/null
  cd $directory
 
  libs=`ls *so 2>/dev/null`;
  for i in $libs; do
        install_name_tool -change $old_prefix $new_prefix $i
  done
 
  popd > /dev/null
}
 
update_config()
{
  directory=$1
  config_file=$2
  old_prefix=$3
  new_prefix=$4
 
  [ ! -d $directory ] && return
  pushd . > /dev/null
  cd $directory
 
  mv $config_file $config_file".old"
  sed -e "s,$old_prefix,$new_prefix," ./$config_file".old" > $config_file
  rm $config_file".old"
 
  popd > /dev/null
}

fix_bin() {
	# we need to be in that dir I think
	cd "$(dirname "$1")"
	app="$(basename "$1")"
	
	bin="${app/.app/}"
	
	echo ">>> building file list"
	filelist=($app/Contents/Frameworks/*/Resources/dev/lib/**/*)
	filelist=($filelist $app/Contents/Frameworks/*/Resources/lib/**/*)
	filelist=($filelist $app/Contents/Frameworks/*/Libraries/**/*)
	filelist=($filelist $app/Contents/Frameworks/*/*)
	filelist=($filelist $app/Contents/MacOS/*)
	
	echo ">>> filtering file list"
	newlist=()
	for f in $filelist; do
		if [ -L "$f" ] && readlink "$f" | grep "/Library/Frameworks" >/dev/null; then
			link=$(readlink "$f" | sed -e "s|/Library/Frameworks/||")
			relf=$(echo "$f" | sed -e "s|$app/Contents/Frameworks/||")
			relf="$(dirname "$relf" | sed -e "s|[^/]*|\.\.|g")"
			ln -sf "$relf/$link" $f
			continue
		fi
		file "$f" | grep "Mach-O" >/dev/null && \
		otool -L "$f" 2>/dev/null | grep "/Library/Frameworks" >/dev/null && \
		newlist=($newlist $f)
	done
	filelist=($newlist)
		
	echo ">>> fixing libs"
	for f in $filelist; do
		badlinks=($(otool -L "$f" | cut -f 1 -d " " | egrep "^[[:space:]]*\/Library\/Frameworks/.*" | egrep -o "\/Library\/.*"))

		for link in $badlinks; do
			fixedlink=$(echo $link | sed -e "s|/Library/Frameworks|@executable_path/../Frameworks|")
			install_name_tool -change $link $fixedlink $f
		done
	done


	echo ">>> fixing configs"
	frameworks=(Cairo Gtk GLib)
	for fr in $frameworks; do		
		prefix="/Library/Frameworks"
		new_prefix="@executable_path/../Frameworks"
		new_framework="$app/Contents/Frameworks/$fr.framework"
		
		# 4. Update pango modules
		update_config "$new_framework/Resources/etc/pango" "pango.modules" $prefix $new_prefix
		 
		# 5. Update GTK+ modules
		update_config "$new_framework/Resources/etc/gtk-2.0/" "gdk-pixbuf.loaders" $prefix $new_prefix
		update_config "$new_framework/Resources/etc/gtk-2.0/" "gtk.immodules" $prefix $new_prefix
	done
	
}


if [ "$1" != "" ]; then
	fix_bin "$1"
	exit $?
fi

cd build

for rel in *; do
	cd $rel
	[ -e *.app ] && \
	for app in *.app; do
		fix_app "$app"
	done
	cd ..
done


