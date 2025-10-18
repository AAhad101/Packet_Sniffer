# Fighting for the resources
Assumptions:
    - Let's say the store is crowded: there's more than 4 customers. One of the customers leaves at timestamp t, I assumed that a customer standing can replace him on th sofa at the same timestamp t.

    E.g: 16 Customer 3214 enters    (Customer 3214 can't sit since the sofas are full)
         18 Customer 1234 leaves    (Customer 1234 leaves so Customer 3214 can sit)
         18 Customer 3214 sits      (Doesn't wait for the next timestamp, instead sits at the same timestamp Customer 1234 left)

    The reasoning behind this assumption of mine is that when a cashing counter gets free at timestamp t, another customer's payment can get accepted immediately, at the same timestamp t. This can be seen from the "Possible Output Flow" of Example 1 given in the writeup as follows:

    10 Customer 1 enters
    11 Customer 2 enters
    11 Customer 1 sits
    12 Customer 2 sits
    12 Customer 1 requests cake
    12 Customer 3 enters
    13 Chef 2 bakes for Customer 1
    13 Customer 3 sits
    13 Customer 2 requests cake
    14 Chef 1 bakes for Customer 2
    14 Customer 3 requests cake
    15 Customer 1 pays
    15 Chef 3 bakes for Customer 3
    16 Customer 2 pays
    16 Chef 2 accepts payment for Customer 1
    17 Customer 3 pays
    18 Customer 1 leaves
    18 Chef 1 accepts payment for Customer 2
    20 Customer 2 leaves
    20 Chef 3 accepts payment for Customer 3
    22 Customer 3 leaves

    Here at timestamp 18 Customer 1 leaves and at the same timestamp, 18, Chef 1 accepts payment for Customer 2.
