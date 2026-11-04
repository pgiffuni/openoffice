import org.mozilla.javascript.*;
import org.mozilla.javascript.xmlimpl.XMLLibImpl;
import java.io.*;
import java.net.*;
import java.nio.file.Files;
import java.util.concurrent.*;

/**
 * XXE regression tests for Rhino E4X XML processing.
 * Verifies that external resource access is blocked through all XML paths.
 */
public class TestXXEResgression {
    static int passed = 0;
    static int failed = 0;

    static void check(String name, boolean condition, String detail) {
        if (condition) { passed++; System.out.println("PASS: " + name); }
        else { failed++; System.out.println("FAIL: " + name + " (detail: " + detail + ")"); }
    }

    public static void main(String[] args) throws Exception {
        Context cx = Context.enter();
        try {
            Scriptable scope = cx.initStandardObjects();
            XMLLibImpl.init(cx, scope, false);

            // ---------------------------------------------------------------
            // Test 1: External general entity referencing a local file
            // ---------------------------------------------------------------
            File tempFile = File.createTempFile("xxe-test-", ".txt");
            tempFile.deleteOnExit();
            Files.write(tempFile.toPath(), "SECRET_FILE_CONTENT".getBytes());
            String fileUri = tempFile.toURI().toString();

            String xxe1 = "var x = XML('<!DOCTYPE root [<!ENTITY ext SYSTEM \"FILE_URI\">]<root>&ext;</root>'); x.toString();";
            xxe1 = xxe1.replace("FILE_URI", fileUri);
            Object r1 = eval(cx, scope, xxe1);
            String s1 = Context.toString(r1);
            check("External general entity (local file) not disclosed or errors",
                  !s1.contains("SECRET_FILE_CONTENT"), s1);

            // ---------------------------------------------------------------
            // Test 2: External general entity referencing local HTTP listener
            // ---------------------------------------------------------------
            ExecutorService httpExec = Executors.newSingleThreadExecutor();
            final boolean[] httpRequested = {false};
            ServerSocket server = new ServerSocket(0);
            final int port = server.getLocalPort();
            httpExec.submit(() -> {
                try {
                    Socket s = server.accept();
                    httpRequested[0] = true;
                    PrintWriter pw = new PrintWriter(s.getOutputStream());
                    pw.print("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
                    pw.flush();
                    s.close();
                } catch (IOException e) {}
            });

            String xxe2 = "var x = XML('<!DOCTYPE root [<!ENTITY ext SYSTEM \"http://127.0.0.1:PORT/test\">]<root>&ext;</root>'); x.toString();";
            xxe2 = xxe2.replace("PORT", String.valueOf(port));
            Object r2 = eval(cx, scope, xxe2);
            String s2 = Context.toString(r2);
            Thread.sleep(500);
            check("External general entity (HTTP) no outbound request", !httpRequested[0], "");
            check("External general entity (HTTP) parsing blocked or empty", !s2.contains("SECRET"), s2);
            server.close();
            httpExec.shutdownNow();

            // ---------------------------------------------------------------
            // Test 3: External parameter entity
            // ---------------------------------------------------------------
            String xxe3 = "var x = XML('<!DOCTYPE root [<!ENTITY % ext SYSTEM \"FILE_URI\">%ext;]<root/>'); x.toString();";
            xxe3 = xxe3.replace("FILE_URI", fileUri);
            Object r3 = eval(cx, scope, xxe3);
            String s3 = Context.toString(r3);
            check("External parameter entity not disclosed",
                  !s3.contains("SECRET_FILE_CONTENT"), s3);

            // ---------------------------------------------------------------
            // Test 4: External DTD
            // ---------------------------------------------------------------
            File dtdFile = File.createTempFile("xxe-dtd-", ".dtd");
            dtdFile.deleteOnExit();
            Files.write(dtdFile.toPath(), "<!ENTITY ext2 \"SECRET_DTD_CONTENT\">".getBytes());
            String dtdUri = dtdFile.toURI().toString();
            String xxe4 = "var x = XML('<!DOCTYPE root SYSTEM \"DTD_URI\"><root>&ext2;</root>'); x.toString();";
            xxe4 = xxe4.replace("DTD_URI", dtdUri);
            Object r4 = eval(cx, scope, xxe4);
            String s4 = Context.toString(r4);
            check("External DTD not disclosed", !s4.contains("SECRET_DTD_CONTENT"), s4);

            // ---------------------------------------------------------------
            // Test 5: XInclude
            // ---------------------------------------------------------------
            String xxe5 = "var x = XML('<root xmlns:xi=\"http://www.w3.org/2001/XInclude\"><xi:include href=\"FILE_URI\"/></root>'); x.toString();";
            xxe5 = xxe5.replace("FILE_URI", fileUri);
            Object r5 = eval(cx, scope, xxe5);
            String s5 = Context.toString(r5);
            check("XInclude not resolved", !s5.contains("SECRET_FILE_CONTENT"), s5);

            // ---------------------------------------------------------------
            // Test 6: Internal entities and ordinary XML documents
            // DOCTYPE with internal entities is blocked by secure processing.
            // E4X (ECMA-357) does not require DOCTYPE, so this is acceptable.
            // ---------------------------------------------------------------
            String internal1 = "var x = XML('<!DOCTYPE root [<!ENTITY ent \"internal_value\">]<root>&ent;</root>'); x.toString();";
            Object r6 = eval(cx, scope, internal1);
            String s6 = Context.toString(r6);
            check("Internal entity in DOCTYPE does not disclose external resources",
                  !s6.contains("SECRET") && (s6.contains("internal_value") || s6.contains("Error") || s6.contains("Cannot parse")), s6);

            String ordinary = "var x = XML('<root><child attr=\"val\">text</child></root>'); x.toString();";
            Object r7 = eval(cx, scope, ordinary);
            String s7 = Context.toString(r7);
            check("Ordinary XML parses", s7.contains("root") && s7.contains("child"), s7);

            // ---------------------------------------------------------------
            // Test 7: Normal E4X parsing and serialization
            // ---------------------------------------------------------------
            String e4x1 = "var x = <root><child>hello</child></root>; x.child.toString();";
            Object r8 = eval(cx, scope, e4x1);
            String s8 = Context.toString(r8);
            check("E4X literal parsing", s8.contains("hello"), s8);

            String e4x2 = "var x = <root/>; x.appendChild(<child>world</child>); x.child.toString();";
            Object r9 = eval(cx, scope, e4x2);
            String s9 = Context.toString(r9);
            check("E4X serialization", s9.contains("world"), s9);

            // ---------------------------------------------------------------
            // Test 8: Malformed XML and parser error handling
            // ---------------------------------------------------------------
            String malformed = "var x = XML('<root><unclosed>'); x.toString();";
            Object r10 = eval(cx, scope, malformed);
            String s10 = Context.toString(r10);
            check("Malformed XML throws error",
                  s10.contains("Error") || s10.contains("Exception") || s10.contains("Cannot parse"), s10);

            // ---------------------------------------------------------------
            // Summary
            // ---------------------------------------------------------------
            System.out.println("\n=== XXE Results: " + passed + " passed, " + failed + " failed ===");
            tempFile.delete();
            dtdFile.delete();
        } finally {
            Context.exit();
        }
        if (failed > 0) System.exit(1);
    }

    static Object eval(Context cx, Scriptable scope, String expr) {
        try {
            return cx.evaluateString(scope, expr, "test", 1, null);
        } catch (Exception e) {
            return "ERROR: " + e.getMessage();
        }
    }
}
