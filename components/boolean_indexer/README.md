# be indexer implement

ref:[Indexing-Boolean-Expressions](./doc/Indexing-Boolean-Expressions.pdf)

## terms

- field:
  a name for every group of attributions
  eg: user, ip, sex, tag

- Assigns:
  a field with multi(or just one) value[s]
  eg: "user": [david, kevin, lisa]

- Attr:
  a field with a it's value, eg: `<user, david>, <ip, localhost>`

- BooleanExpr:
  represent a boolean expression, it contain a Assigns and a boolean flag mean include/exclude

  eg: "user" include [david, kevin, lisa]
  it can descript as follow:
  (david, kevin, lisa) any of them for this expression will be true, false for any other user in this expression

- Conjunction:
  contain many BooleanExpr with a `id` and `size`
  the size mean the count of BooleanExpr with flag "include"
```text
  eg:
    id: 10,
    size: 2,
    Expressions: [
      "os" exclude [linux, windows] # size 0 bz of flag exclude
      "tag"  include [male, rich]   # size 1
      "user" include [david, lisa]  # size 1
    ]
```

- Document:
  contain one or more conjunction
```text
  doc: 1
    {
       id: 10,
       size: 2,
       Expressions: [
         "os" exclude [linux]
         "tag"  include [male, rich]
         "user" include [david, lisa]
       ]
    }, # or logic between many conjunctions
    {
       id: 11,
       size: 1,
       Expressions: [
         "os" include [ios]
         "ip"  exclude [127.0.0.1, 192.168.*.*]
       ]
    }
```

- EntryId:
  a unique id can represent doc + conj + size + flag

- PostingList:
```text
  Attr: EntryIdList
eg:
  <age, 15>: [entry_id_01, entry_id_11, entry_id_15]
```

- PostingListGroup:
```text
  PostingList list group by field
  eg:
    ip:
    <ip, 127.0.0.1>: [entry_id_11, entry_id_15]
    <ip, 192.168.28.1>: [entry_id_01, entry_id_11, entry_id_15]
```
