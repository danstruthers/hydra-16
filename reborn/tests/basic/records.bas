' records.bas - TYPE: fields of numbers, strings, fixed-length strings and other records; DIM AS a type; arrays of
' them; one copied whole; a record, an element and a field given to a procedure
DIM SHARED checks, failed
SUB ck (got, want, what$)
    checks = checks + 1
    IF got <> want THEN failed = failed + 1: PRINT "FAIL "; what$; ":"; got; "not"; want
END SUB

TYPE point
    x AS INTEGER
    y AS INTEGER
END TYPE
TYPE shape
    kind AS STRING * 6
    name AS STRING
    at AS point
    size AS DOUBLE
END TYPE

DIM p AS point, q AS point
ck p.x + p.y, 0, "0 at the start"
p.x = 3: p.y = 4
ck p.x * 10 + p.y, 34, "fields"
q = p
q.x = 9
ck p.x * 10 + q.x, 39, "a copy, its own"
DIM s AS shape
s.kind = "circle!!"
s.name = "the sun"
s.at.x = 100: s.at.y = 1 / 3
s.size = 2 ^ 80
ck s.kind = "circle", -1, "STRING * 6: cut"
ck LEN(s.kind), 6, "its length"
ck s.name = "the sun", -1, "a string field"
ck s.at.x + s.at.y * 3, 101, "a record in it"
ck s.size, 2 ^ 80, "big"
s.at = p
ck s.at.y, 4, "a field given a record"
DIM t AS shape
t = s
s.at.x = 0: s.name = "changed"
ck t.at.x, 3, "a copy of the nested record"
ck t.name = "the sun", -1, "a copy of the string"
' Arrays of records
DIM path(1 TO 5) AS point
FOR i = 1 TO 5: path(i).x = i: path(i).y = i * i: NEXT
sum = 0
FOR i = 1 TO 5: sum = sum + path(i).x + path(i).y: NEXT
ck sum, 70, "an array of records"
path(2) = p
ck path(2).x * 10 + path(2).y, 34, "an element given a record"
q = path(5)
ck q.y, 25, "a record given an element"
DIM shapes(3) AS shape
shapes(1).at.x = 7: shapes(1).name = "one"
ck shapes(1).at.x, 7, "a nested field in an element"
ck shapes(1).name = "one", -1, "a string field in an element"
ck LEN(shapes(2).kind), 6, "fixed: spaces"
' Procedures
SUB moveBy (pt AS point, dx, dy)
    pt.x = pt.x + dx
    pt.y = pt.y + dy
END SUB
FUNCTION dist2 (a AS point, b AS point)
    dist2 = (a.x - b.x) ^ 2 + (a.y - b.y) ^ 2
END FUNCTION
p.x = 1: p.y = 1
moveBy p, 2, 3
ck p.x * 10 + p.y, 34, "a record by reference"
moveBy path(1), 10, 10
ck path(1).x, 11, "an element by reference"
SUB dbl (n)
    n = n * 2
END SUB
dbl p.x
ck p.x, 6, "a field by reference"
dbl path(3).y
ck path(3).y, 18, "an element's field by reference"
SUB dblIn (pt AS point)
    dbl pt.x
END SUB
p.x = 4: dblIn p
ck p.x, 8, "a parameter's field by reference"
p.x = 6
q.x = 0: q.y = 0
ck dist2(p, q), 52, "records to a FUNCTION"
PRINT "records:"; checks; "checks,"; failed; "failed"
