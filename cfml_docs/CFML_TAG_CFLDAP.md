# Tag Name: `cfldap`

## Description
Provides an interface to a Lightweight Directory Access Protocol
 (LDAP) directory server, such as the Netscape Directory Server.

## Syntax
```cfml
<cfldap server="">
```

## Attributes / Variants

### Attribute: `server`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Host name or IP address of LDAP server.

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `389`
- **Description**: Port of the LDAP server (default 389).

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The User ID. Required if secure = "CFSSL_BASIC"

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password that corresponds to user name.
 If secure = "CFSSL_BASIC", V2 encrypts the password before
 transmission.

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `query`
- **Description**: * query: returns LDAP entry information only. Requires name,
 start, and attributes attributes.
 * add: adds LDAP entries to LDAP server. Requires attributes
 attribute.
 * modify: modifies LDAP entries, except distinguished name dn
 attribute, on LDAP server. Requires dn. See modifyType attribute.
 * modifyDN: modifies distinguished name attribute for LDAP
 entries on LDAP server. Requires dn.
 * delete: deletes LDAP entries on an LDAP server. Requires dn.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Required if action = "Query"
 Name of LDAP query. The tag validates the value.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `60000`
- **Description**: Maximum length of time, in milliseconds, to wait for LDAP processing.
 Default 60000

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of entries for LDAP queries.

### Attribute: `start`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Required if action = "Query"
 Distinguished name of entry to be used to start a search.

### Attribute: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `onelevel`
- **Description**: Scope of search, from entry specified in start attribute for
 action = "Query".
 * oneLevel: entries one level below entry.
 * base: only the entry.
 * subtree: entry and all levels below it.

### Attribute: `attributes`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Required if action = "Query", "Add", "ModifyDN", or "Modify"
 For queries: comma-delimited list of attributes to return. For
 queries, to get all attributes, specify "*".
 
 If action = "add" or "modify", you can specify a list of update
 columns. Separate attributes with a semicolon.
 
 If action = "ModifyDN", CFML passes attributes to the
 LDAP server without syntax checking.

### Attribute: `returnasbinary`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF7+ A comma-delimited list of columns that are to
 be returned as binary values.

### Attribute: `filter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Search criteria for action = "Query".
 List attributes in the form:
 "(attribute operator value)" Example: "(sn = Smith)"

### Attribute: `sort`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Attribute(s) by which to sort query results. Use a comma
 delimiter.

### Attribute: `sortcontrol`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: Default asc
 * nocase: case-insensitive sort
 * asc: ascending (a to z) case-sensitive sort
 * desc: descending (z to a) case-sensitive sort

 You can enter a combination of sort types; for example,
 sortControl = "nocase, asc".

### Attribute: `dn`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Distinguished name, for update action. Example:
 "cn = Bob Jensen, o = Ace Industry, c = US"

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Used with action = "query". First row of LDAP query to insert
 into a CFML query.

### Attribute: `modifytype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `replace`
- **Description**: Default replace

 How to process an attribute in a multi-value list.
 * add: appends it to any attributes
 * delete: deletes it from the set of attributes
 * replace: replaces it with specified attributes

 You cannot add an attribute that is already present or that is
 empty.

### Attribute: `rebind`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: * Yes: attempt to rebind referral callback and reissue query by
 referred address using original credentials.
 * No: referred connections are anonymous

### Attribute: `referral`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of hops allowed in a referral. A value of 0 disables
 referred addresses for LDAP; no data is returned.

### Attribute: `secure`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Security to employ, and required information. One option:
 * CFSSL_BASIC

 "CFSSL_BASIC" provides V2 SSL encryption
 and server authentication.

### Attribute: `separator`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Default , (a comma)
 Delimiter to separate attribute values of multi-value
 attributes. Used by query, add, and modify actions, and by
 cfldap to output multi-value attributes.

 For example, if $ (dollar sign), the attributes attribute could
 be "objectclass = top$person", where the first value of
 objectclass is top, and the second value is person. This avoids
 confusion if values include commas.

### Attribute: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `;`
- **Description**: Separator between attribute name-value pairs. Use this
 attribute if:

 * the attributes attribute specifies more than one item, or
 * an attribute contains the default delimiter (semicolon). For
 example: mgrpmsgrejecttext;lang-en

 Used by query, add, and modify actions, and by cfldap to output
 multi-value attributes.

 For example, if $ (dollar sign), you could specify
 "cn = Double Tree Inn$street = 1111 Elm; Suite 100 where the
 semicolon is part of the street value.

### Attribute: `clientcert`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ A file path to a client certificate.

### Attribute: `clientcertpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ The password for the client certificate file.

### Attribute: `usetls`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF11+ Indicates that the connection should be made using transport layer security.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

