const libnut = require("bindings")("libnut");
// libnut.setClipboardHTML("<b>ABCD</b>", "plaintext");
// libnut.setClipboardText("text-plain");
libnut.setClipboardHTML('<b>bold </b><em>italic </em><u>underline </u><u><a href="https://google.com">link</a></u>', "bold italic underline link");