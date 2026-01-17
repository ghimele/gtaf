# TPC-H Benchmark for GTAF

This directory contains tools to import and query TPC-H benchmark data using GTAF.

## What is TPC-H?

TPC-H is an industry-standard database benchmark that simulates a wholesale supplier database with 8 related tables and millions of rows. It's widely used to test database performance on complex analytical queries.

For more details, see: [docs/QUERY_OPTIMIZATION_GUIDE.md](../../docs/QUERY_OPTIMIZATION_GUIDE.md)

## Quick Start

### 1. Generate TPC-H Data

First, generate the TPC-H dataset using the official dbgen tool:

```bash
# Clone the TPC-H data generator
git clone https://github.com/gregrahn/tpch-kit.git
cd tpch-kit/dbgen

# Build the generator
make

# Generate data (Scale Factor 1 = ~1 GB, 6M rows in lineitem)
./dbgen -s 1

# This creates 8 .tbl files:
# - region.tbl (5 rows)
# - nation.tbl (25 rows)
# - supplier.tbl (10,000 rows)
# - customer.tbl (150,000 rows)
# - part.tbl (200,000 rows)
# - partsupp.tbl (800,000 rows)
# - orders.tbl (1,500,000 rows)
# - lineitem.tbl (6,001,215 rows)
```

**Scale Factors**:
- SF1: 1 GB (~6 million rows in lineitem)
- SF10: 10 GB (~60 million rows) - Good for stress testing
- SF100: 100 GB (~600 million rows) - Large scale testing

### 2. Build GTAF Tools

```bash
cd /path/to/gtaf
cmake --build build

# This builds:
# - build/gtaf_tpch_import  (imports TPC-H data to GTAF)
# - build/gtaf_tpch_query   (runs queries on imported data)
```

### 3. Import TPC-H Data into GTAF

```bash
# Import all TPC-H tables
./build/gtaf_tpch_import /path/to/tpch-kit/dbgen tpch_sf1.dat

# Expected output:
# Importing REGION from: /path/to/tpch-kit/dbgen/region.tbl
#   ✓ Imported 5 regions
# Importing NATION from: /path/to/tpch-kit/dbgen/nation.tbl
#   ✓ Imported 25 nations
# ...
# Importing LINEITEM from: /path/to/tpch-kit/dbgen/lineitem.tbl
#   ✓ Imported 6,001,215 line items
#
# === Import Summary ===
# Total rows imported: 8,661,245
# Import time: ~60-120 seconds
# Memory used: ~2-4 GB
```

### 4. Run Queries

```bash
# Query the imported data
./build/gtaf_tpch_query tpch_sf1.dat

# Expected output:
# === TPC-H Query 1: Pricing Summary Report ===
# Found 5,998,123 matching line items
# Query time: 50-200ms
#
# === TPC-H Query 3: Shipping Priority ===
# Found 30,142 customers in BUILDING segment
# Query time: 15-50ms
```

## Performance Expectations

### Scale Factor 1 (SF1)

| Metric | Expected Value |
|--------|---------------|
| **Dataset Size** | ~1 GB raw, ~800 MB imported |
| **Total Rows** | 8.66 million |
| **Import Time** | 60-120 seconds |
| **Import Memory** | 2-4 GB |
| **Index Build Time** | 30-60 seconds |
| **Query Time** | 50-500ms |
| **Final Memory** | 1-2 GB |

### Scale Factor 10 (SF10)

| Metric | Expected Value |
|--------|---------------|
| **Dataset Size** | ~10 GB raw, ~8 GB imported |
| **Total Rows** | 86.6 million |
| **Import Time** | 10-20 minutes |
| **Import Memory** | 20-40 GB |
| **Index Build Time** | 5-10 minutes |
| **Query Time** | 100-1000ms |
| **Final Memory** | 10-20 GB |

## TPC-H Schema

```
REGION (5 rows)
  ├─ NATION (25 rows)
      ├─ CUSTOMER (150K rows)
      │   └─ ORDERS (1.5M rows)
      │       └─ LINEITEM (6M rows) [LARGEST]
      └─ SUPPLIER (10K rows)
          └─ PARTSUPP (800K rows)
              └─ PART (200K rows)
```

### Table Details

**LINEITEM** (16 columns, 6M rows @ SF1):
- `orderkey`, `partkey`, `suppkey`, `linenumber`
- `quantity`, `extendedprice`, `discount`, `tax`
- `returnflag`, `linestatus`
- `shipdate`, `commitdate`, `receiptdate`
- `shipinstruct`, `shipmode`, `comment`

**ORDERS** (9 columns, 1.5M rows):
- `orderkey`, `custkey`, `orderstatus`
- `totalprice`, `orderdate`, `orderpriority`
- `clerk`, `shippriority`, `comment`

**CUSTOMER** (8 columns, 150K rows):
- `custkey`, `name`, `address`, `nationkey`
- `phone`, `acctbal`, `mktsegment`, `comment`

**PART** (9 columns, 200K rows):
- `partkey`, `name`, `mfgr`, `brand`
- `type`, `size`, `container`, `retailprice`, `comment`

**SUPPLIER** (7 columns, 10K rows):
- `suppkey`, `name`, `address`, `nationkey`
- `phone`, `acctbal`, `comment`

## Sample Queries

### Query 1: Pricing Summary Report

```sql
SELECT
    l_returnflag,
    l_linestatus,
    SUM(l_quantity) as sum_qty,
    SUM(l_extendedprice) as sum_base_price,
    AVG(l_quantity) as avg_qty
FROM lineitem
WHERE l_shipdate <= date '1998-12-01' - interval '90' day
GROUP BY l_returnflag, l_linestatus
ORDER BY l_returnflag, l_linestatus;
```

**GTAF equivalent**:
```cpp
// Find lineitems with shipdate <= threshold
auto matches = index.find_int_where("lineitem.shipdate", [](int64_t date) {
    return date <= 19980902;  // 1998-09-02
});

// Group by returnflag and linestatus
std::map<std::pair<std::string, std::string>, AggregateData> groups;
for (const auto& entity : matches) {
    auto flag = index.get_string("lineitem.returnflag", entity);
    auto status = index.get_string("lineitem.linestatus", entity);
    auto qty = index.get_string("lineitem.quantity", entity);

    groups[{*flag, *status}].add(std::stod(*qty));
}
```

### Query 3: Shipping Priority

```sql
SELECT
    l_orderkey,
    SUM(l_extendedprice * (1 - l_discount)) as revenue
FROM customer, orders, lineitem
WHERE c_mktsegment = 'BUILDING'
  AND c_custkey = o_custkey
  AND l_orderkey = o_orderkey
GROUP BY l_orderkey
ORDER BY revenue DESC
LIMIT 10;
```

**GTAF equivalent**:
```cpp
// Step 1: Find customers in BUILDING segment
auto customers = index.find_equals("customer.mktsegment", "BUILDING");

// Step 2: Find their orders (would need to implement join logic)
// Step 3: Find line items for those orders
// Step 4: Calculate revenue and group
```

## GTAF Performance Features Tested

### 1. Entity Deduplication
- TPC-H has many repeated values (same suppliers, regions, etc.)
- GTAF's content-based deduplication should show 30-50% deduplication rate

### 2. Query Index Performance
- Queries scan millions of rows
- Indexes enable sub-second query times
- Tests scalability of index system

### 3. Multi-Table Relationships
- 8 related tables with foreign keys
- Tests how GTAF handles related entities
- Validates entity reference architecture

### 4. Temporal Data
- Orders and shipments have dates
- Tests GTAF's temporal chunk management
- Validates time-series query patterns

## Troubleshooting

### "Cannot open .tbl file"
Make sure you specify the correct directory containing the .tbl files:
```bash
./build/gtaf_tpch_import /full/path/to/tpch-kit/dbgen
```

### Out of Memory during Import
If importing SF10 or larger:
1. Ensure you have enough RAM (SF10 needs ~40 GB during import)
2. Consider processing in smaller chunks
3. Use a machine with more memory

### Slow Query Performance
1. Make sure indexes are built before querying
2. Check memory usage - swapping will slow down queries
3. Consider using a smaller scale factor for testing

## Comparing GTAF vs Traditional Databases

You can compare GTAF's performance against other databases using the same TPC-H data:

1. **PostgreSQL**: Use `pg_bulkload` to import .tbl files
2. **MySQL**: Use `LOAD DATA INFILE` to import .tbl files
3. **ClickHouse**: Use their TPC-H import scripts
4. **DuckDB**: Use DuckDB's TPC-H extension

Compare:
- Import time
- Query time
- Memory usage
- Deduplication effectiveness

## Next Steps

After validating GTAF with TPC-H:

1. **Implement TPC-H Queries 1-22**: Complete the full benchmark suite
2. **Add JOIN support**: Implement multi-table joins in QueryIndex
3. **Optimize Import**: Parallel import of tables
4. **Add Aggregations**: SUM, AVG, COUNT in QueryIndex
5. **Persistence**: Save/load pre-built indexes

## References

- [TPC-H Official Specification](https://www.tpc.org/tpch/)
- [TPC-H Data Generator](https://github.com/gregrahn/tpch-kit)
- [GTAF Query Optimization Guide](../../docs/QUERY_OPTIMIZATION_GUIDE.md)
- [GTAF Query Index Documentation](../../docs/query-index-optimization.md)
