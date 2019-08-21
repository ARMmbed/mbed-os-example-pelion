@Library('get-targets')_

pipeline {
    agent any
    options {
        timestamps()
    }

    stages {
        stage('Get targets') {
            steps {
                get-targets("mbed_app.json")
            }
        }
    }
}

