component {
    property name="fullname" type="string" default="NOBODY";
    property name="title" type="string" default="MR";

    this.appName = "ModernPerson";
    this.version = 1;

    public function init(name) {
        this.name = arguments.name;
        return this;
    }

    public function getName() {
        return this.name;
    }

    private function secret() {
        return "PRIVATE_SECRET";
    }

    public function reveal() {
        return secret();
    }

    public string function greet(who="world") {
        return "Hello " & arguments.who;
    }

    public numeric function add(a, b) {
        return arguments.a + arguments.b;
    }

    public function echoThis() {
        return this;
    }
}
