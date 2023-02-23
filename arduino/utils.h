void serial_printf(char* fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 512, fmt, args);
  va_end(args);
  Serial.print(buf);
}

String urlEncode(const char* message) {
  static const char hex[] = "0123456789abcdef";

  String encodedMessage = "";
  while(*message != '\0') {
    if( ('a' <= *message && *message <= 'z') || ('A' <= *message && *message <= 'Z') || ('0' <= *message && *message <= '9') ) {
      encodedMessage += *message;
    } else {
      encodedMessage += '%';
      encodedMessage += hex[*message >> 4];
      encodedMessage += hex[*message & 15];
    }
    message++;
  }

  return encodedMessage;
}