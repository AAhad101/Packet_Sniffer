#include "kernel/types.h"
#include "user.h"

// stackgrow: provoke valid stack growth then invalid deep access
static void touch_stack_bytes(int bytes) {
  volatile char buf[2048];
  for (int i = 0; i < sizeof(buf); i += 64)
    ((volatile char*)buf)[i] = (char)i;
  if (bytes > (int)sizeof(buf)) {
    // Recurse to grow stack more but keep under one page first
    touch_stack_bytes(bytes - sizeof(buf));
  }
}

int main(int argc, char **argv) {
  // 1) Valid growth: touch less than one page below current SP
  touch_stack_bytes(3000); // < 4096
  printf("stackgrow: valid growth done\n");

  // 2) Invalid: attempt to access far below SP.
  // We try to create a new frame with a large local array to jump >1 page.
  // If your kernel enforces only one-page slack, this should KILL invalid-access.
  volatile char big[8192];
  for (int i = 0; i < 8192; i += 512)
    ((volatile char*)big)[i] = (char)i;

  // If not killed, we report and exit 0 so you can see behavior.
  printf("stackgrow: unexpected survival after deep stack touch\n");
  exit(0);
}
