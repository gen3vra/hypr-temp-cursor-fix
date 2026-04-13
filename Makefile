TARGET = cursor-fix.so

all: $(TARGET)

$(TARGET): main.cpp
	g++ -shared -fPIC --no-gnu-unique main.cpp -o $(TARGET) \
		`pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon` \
		-std=c++2b -O2

clean:
	rm -f $(TARGET)