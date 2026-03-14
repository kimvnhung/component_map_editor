# QmlFontAwesome
==============

Reusable Qt 6 library that exposes Font Awesome icons for both C++ and QML.

## What this module provides
-------------------------

- QML module URI: `QmlFontAwesome`
- QML singleton API: `FontAwesome`
- QML convenience component: `FaIcon`
- C++ API via header: `FontAwesome.h`

The module embeds:

- `fontawesome/metadata/icons.json`
- OTF files for solid, regular, and brands styles

## Build inside a project (add_subdirectory)
------------------------------------------

```cmake
add_subdirectory(path/to/qml_fontawesome)
target_link_libraries(your_app PRIVATE QmlFontAwesome::QmlFontAwesome)
```

## Install and consume as package
------------------------------

```bash
cmake -S qml_fontawesome -B build/qml_fontawesome
cmake --build build/qml_fontawesome
cmake --install build/qml_fontawesome --prefix /your/install/prefix
```

Then in another project:

```cmake
find_package(QmlFontAwesome REQUIRED)
target_link_libraries(your_app PRIVATE QmlFontAwesome::QmlFontAwesome)
```
## How to use
### Download fontawesome from web
https://fontawesome.com/download
### QML usage
---------

```qml
import QtQuick
import QmlFontAwesome

Row {
	spacing: 8

	FaIcon {
		iconName: "house"
		iconStyle: FontAwesome.Solid
		font.pixelSize: 20
		color: "#2d7ef7"
	}

	Text {
		text: FontAwesome.icon("github", FontAwesome.Brands)
		font.family: FontAwesome.family(FontAwesome.Brands)
		font.weight: FontAwesome.weight(FontAwesome.Brands)
		font.pixelSize: 20
	}
}
```

### Search + favorites/recent cache

```qml
import QtQuick
import QmlFontAwesome

Item {
	Component.onCompleted: {
		const matches = FontAwesome.searchIcons("arrow", 20)
		if (matches.length > 0) {
			FontAwesome.addFavorite(matches[0])
			FontAwesome.markIconUsed(matches[0])
		}

		console.log("Favorites:", FontAwesome.favoriteIcons())
		console.log("Recent:", FontAwesome.recentIcons())
	}
}
```

Favorites and recent icons are persisted via `QSettings`.

### C++ usage
---------

```cpp
#include <FontAwesome.h>

FontAwesome::initialize();

const QString iconText = FontAwesome::iconForCpp("house", FontAwesome::Solid);
const QString family = FontAwesome::familyForCpp(FontAwesome::Solid);
const int weight = FontAwesome::weightForCpp(FontAwesome::Solid);
```

Supported style values
----------------------

- `FontAwesome.Solid`
- `FontAwesome.Regular`
- `FontAwesome.Brands`

Also accepted by `iconByStyleName(...)`: `solid`, `regular`, `brands`, `fas`, `far`, `fab`.

