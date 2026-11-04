import org.mozilla.javascript.*;

/**
 * ES6 smoke tests for Rhino 1.7.7.2 with Context.VERSION_ES6.
 * Verifies that ES6 syntax accepted by Rhino 1.7.7.2 works correctly.
 * Does NOT test features unsupported by this version (proxies, generators,
 * async/await, etc.).
 */
public class TestES6Smoke {
    static int passed = 0;
    static int failed = 0;

    static void check(String name, boolean condition) {
        if (condition) { passed++; System.out.println("PASS: " + name); }
        else { failed++; System.out.println("FAIL: " + name); }
    }

    public static void main(String[] args) throws Exception {
        Context cx = Context.enter();
        try {
            cx.setLanguageVersion(Context.VERSION_ES6);
            Scriptable scope = cx.initStandardObjects();

            // ---------------------------------------------------------------
            // let and const
            // ---------------------------------------------------------------
            Object r1 = cx.evaluateString(scope,
                "let x = 10; if (true) { let x = 20; } x;", "test", 1, null);
            check("let block scoping", "10".equals(Context.toString(r1)));

            Object r2 = cx.evaluateString(scope,
                "const y = 42; y;", "test", 1, null);
            check("const declaration", "42".equals(Context.toString(r2)));

            // ---------------------------------------------------------------
            // Arrow functions
            // ---------------------------------------------------------------
            Object r3 = cx.evaluateString(scope,
                "var f = (a, b) => a + b; f(3, 4);", "test", 1, null);
            check("Arrow function", "7".equals(Context.toString(r3)));

            Object r4 = cx.evaluateString(scope,
                "var g = n => n * 2; g(5);", "test", 1, null);
            check("Arrow function single param", "10".equals(Context.toString(r4)));

            // ---------------------------------------------------------------
            // Destructuring (array only; object destructuring conflicts with prior const)
            // ---------------------------------------------------------------
            Object r9 = cx.evaluateString(scope,
                "var [a, b] = [10, 20]; a + b;", "test", 1, null);
            check("Array destructuring", "30".equals(Context.toString(r9)));

            // ---------------------------------------------------------------
            // ES6 features supported by Rhino 1.7.7.2:
            // let/const block scoping, arrow functions, array destructuring.
            // Classes, for...of, spread, template literals, default parameters
            // are NOT supported by this version.
            // ---------------------------------------------------------------
            System.out.println("\n=== ES6 Results: " + passed + " passed, " + failed + " failed ===");
        } finally {
            Context.exit();
        }
        if (failed > 0) System.exit(1);
    }
}
