node {
    stage('Setup') {
        println("Setup stage!")
    }
    stage("Build") {
        println("Build stage!")
        sh 'ls'
        sh 'make'
    }
    stage("Test") {
        println("Test Stage!")
    }
    stage("Analyze"){
        println("Analyze Stage!")
    }
}
post {
    always {
        // Clean up tasks
        println("Cleanup tasks")
    }
    success {
        // Actions to take on a successful run
        println("Success!")
    }
    failure {
        // Actions for a failed run
        println("Failure!")
    }
}
