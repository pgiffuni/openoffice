import org.mozilla.javascript.*;
import java.util.concurrent.*;

/**
 * toFixed regression tests for Rhino Number.prototype.toFixed().
 * Verifies security (no DoS for tiny/subnormal values) and correctness
 * (ECMAScript rounding behavior preserved).
 */
public class TestToFixedRegression {
    static int passed = 0;
    static int failed = 0;
    static final long MAX_MS = 5000; // security timeout: toFixed must complete within 5s

    static void check(String name, boolean condition) {
        if (condition) { passed++; System.out.println("PASS: " + name); }
        else { failed++; System.out.println("FAIL: " + name); }
    }

    static Object eval(Context cx, Scriptable scope, String expr) {
        try {
            return cx.evaluateString(scope, expr, "test", 1, null);
        } catch (Exception e) {
            return "ERROR: " + e.getMessage();
        }
    }

    public static void main(String[] args) throws Exception {
        Context cx = Context.enter();
        try {
            Scriptable scope = cx.initStandardObjects();

            // ---------------------------------------------------------------
            // Security: published vulnerable input must be fast
            // ---------------------------------------------------------------
            long t0 = System.nanoTime();
            Object r1 = cx.evaluateString(scope, "(4.47118444E-314).toFixed(2)", "test", 1, null);
            long t1 = System.nanoTime();
            double ms1 = (t1 - t0) / 1_000_000.0;
            check("Published PoC (4.47118444E-314).toFixed(2) fast (<5000ms)", ms1 < MAX_MS);
            check("Published PoC result is 0.00", "0.00".equals(Context.toString(r1)));

            // ---------------------------------------------------------------
            // Subnormal values
            // ---------------------------------------------------------------
            long t2 = System.nanoTime();
            Object r2 = cx.evaluateString(scope, "(5E-324).toFixed(100)", "test", 1, null);
            long t2b = System.nanoTime();
            check("Smallest subnormal toFixed(100) fast (<5000ms)", (t2b - t2) / 1_000_000.0 < MAX_MS);
            check("Smallest subnormal rounds to zero", Context.toString(r2).matches("^0\\.0{100}$"));

            long t3 = System.nanoTime();
            Object r3 = cx.evaluateString(scope, "(-5E-324).toFixed(100)", "test", 1, null);
            long t3b = System.nanoTime();
            check("Negative subnormal toFixed(100) fast (<5000ms)", (t3b - t3) / 1_000_000.0 < MAX_MS);
            check("Negative subnormal rounds to -0.00...0", Context.toString(r3).matches("^-0\\.0{100}$"));

            // ---------------------------------------------------------------
            // Positive and negative zero
            // ---------------------------------------------------------------
            Object r4 = cx.evaluateString(scope, "(0).toFixed(2)", "test", 1, null);
            check("Positive zero toFixed(2) = 0.00", "0.00".equals(Context.toString(r4)));

            Object r5 = cx.evaluateString(scope, "(-0).toFixed(2)", "test", 1, null);
            check("Negative zero toFixed(2) = 0.00", "0.00".equals(Context.toString(r5)));

            Object r6 = cx.evaluateString(scope, "(0).toFixed(0)", "test", 1, null);
            check("Positive zero toFixed(0) = 0", "0".equals(Context.toString(r6)));

            // ---------------------------------------------------------------
            // Values that round to zero at precision 0, 1, 2, 100
            // ---------------------------------------------------------------
            Object r7 = cx.evaluateString(scope, "(0.1).toFixed(0)", "test", 1, null);
            check("0.1 toFixed(0) = 0", "0".equals(Context.toString(r7)));

            Object r8 = cx.evaluateString(scope, "(0.04).toFixed(1)", "test", 1, null);
            check("0.04 toFixed(1) = 0.0", "0.0".equals(Context.toString(r8)));

            Object r9 = cx.evaluateString(scope, "(0.004).toFixed(2)", "test", 1, null);
            check("0.004 toFixed(2) = 0.00", "0.00".equals(Context.toString(r9)));

            Object r10 = cx.evaluateString(scope, "(1E-101).toFixed(100)", "test", 1, null);
            check("1E-101 toFixed(100) rounds to zero", Context.toString(r10).matches("^0\\.0{100}$"));

            Object r10b = cx.evaluateString(scope, "(1E-50).toFixed(100)", "test", 1, null);
            check("1E-50 toFixed(100) does NOT round to zero", !Context.toString(r10b).matches("^0\\.0{100}$"));

            // ---------------------------------------------------------------
            // Rounding boundaries: below, at, above
            // ---------------------------------------------------------------
            Object r11 = cx.evaluateString(scope, "(0.004999999999999999).toFixed(2)", "test", 1, null);
            check("Below 0.005 boundary toFixed(2) = 0.00", "0.00".equals(Context.toString(r11)));

            Object r12 = cx.evaluateString(scope, "(0.005).toFixed(2)", "test", 1, null);
            check("At 0.005 boundary toFixed(2) = 0.01 (round half up)", "0.01".equals(Context.toString(r12)));

            Object r13 = cx.evaluateString(scope, "(0.005000000000000001).toFixed(2)", "test", 1, null);
            check("Above 0.005 boundary toFixed(2) = 0.01", "0.01".equals(Context.toString(r13)));

            Object r14 = cx.evaluateString(scope, "(4.999999999999999E-101).toFixed(100)", "test", 1, null);
            check("Below 5E-101 boundary toFixed(100) = 0.00...0", Context.toString(r14).matches("^0\\.0{100}$"));

            Object r15 = cx.evaluateString(scope, "(5E-101).toFixed(100)", "test", 1, null);
            check("At 5E-101 boundary toFixed(100) = 0.00...01", Context.toString(r15).matches("^0\\.0{99}1$"));

            Object r16 = cx.evaluateString(scope, "(5.000000000000001E-101).toFixed(100)", "test", 1, null);
            check("Above 5E-101 boundary toFixed(100) = 0.00...01", Context.toString(r16).matches("^0\\.0{99}1$"));

            // ---------------------------------------------------------------
            // Ordinary positive and negative rounding
            // ---------------------------------------------------------------
            Object r17 = cx.evaluateString(scope, "(1.234).toFixed(2)", "test", 1, null);
            check("1.234 toFixed(2) = 1.23", "1.23".equals(Context.toString(r17)));

            Object r18 = cx.evaluateString(scope, "(1.235).toFixed(2)", "test", 1, null);
            check("1.235 toFixed(2) = 1.24 (round half up)", "1.24".equals(Context.toString(r18)));

            Object r19 = cx.evaluateString(scope, "(-1.234).toFixed(2)", "test", 1, null);
            check("-1.234 toFixed(2) = -1.23", "-1.23".equals(Context.toString(r19)));

            Object r20 = cx.evaluateString(scope, "(-1.235).toFixed(2)", "test", 1, null);
            check("-1.235 toFixed(2) = -1.24 (round half up, away from zero)", "-1.24".equals(Context.toString(r20)));

            Object r21 = cx.evaluateString(scope, "(2.5).toFixed(0)", "test", 1, null);
            check("2.5 toFixed(0) = 3 (round half up)", "3".equals(Context.toString(r21)));

            Object r22 = cx.evaluateString(scope, "(-2.5).toFixed(0)", "test", 1, null);
            check("-2.5 toFixed(0) = -3 (round half up, away from zero)", "-3".equals(Context.toString(r22)));

            // ---------------------------------------------------------------
            // Large values and 1e21 boundary
            // ---------------------------------------------------------------
            Object r23 = cx.evaluateString(scope, "(1E20).toFixed(2)", "test", 1, null);
            check("1E20 toFixed(2) = 100000000000000000000.00", "100000000000000000000.00".equals(Context.toString(r23)));

            Object r24 = cx.evaluateString(scope, "(1E21).toFixed(2)", "test", 1, null);
            String s24 = Context.toString(r24);
            check("1E21 toFixed(2) uses exponential (>=1e21 boundary)", s24.contains("e") || s24.contains("E"));

            Object r25 = cx.evaluateString(scope, "(9.99E20).toFixed(2)", "test", 1, null);
            check("9.99E20 toFixed(2) fixed format", !Context.toString(r25).contains("e") && !Context.toString(r25).contains("E"));

            // ---------------------------------------------------------------
            // Precision validation: out-of-range values
            // ---------------------------------------------------------------
            boolean rangeErr = false;
            try {
                cx.evaluateString(scope, "(1.5).toFixed(101)", "test", 1, null);
            } catch (Exception e) {
                rangeErr = e.getMessage() != null && e.getMessage().contains("RangeError");
            }
            check("toFixed(101) throws RangeError", rangeErr);

            boolean rangeErr2 = false;
            try {
                cx.evaluateString(scope, "(1.5).toFixed(-21)", "test", 1, null);
            } catch (Exception e) {
                rangeErr2 = e.getMessage() != null && e.getMessage().contains("RangeError");
            }
            check("toFixed(-21) throws RangeError", rangeErr2);

            // ---------------------------------------------------------------
            // Negative tiny values round to negative zero
            // ---------------------------------------------------------------
            Object r28 = cx.evaluateString(scope, "(-4.47118444E-314).toFixed(2)", "test", 1, null);
            check("Negative tiny toFixed(2) = -0.00", "-0.00".equals(Context.toString(r28)));

            // ---------------------------------------------------------------
            // Summary
            // ---------------------------------------------------------------
            System.out.println("\n=== toFixed Results: " + passed + " passed, " + failed + " failed ===");
        } finally {
            Context.exit();
        }
        if (failed > 0) System.exit(1);
    }
}
