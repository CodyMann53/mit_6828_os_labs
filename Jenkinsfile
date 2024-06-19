node {
    stage('Setup') {
        println("Setup stage!")
        checkout poll: false, 
            scm: scmGit(branches: [[name: '*/master']], extensions: [], userRemoteConfigs: [[url: 'https://github.com/CodyMann53/mit_6828_os_labs']])
    }
    stage("Build") {
        println("Build stage!")
        sh 'cd mit_6828_os_labs && make'
    }
    stage("Test") {
        println("Test Stage!")
        sh 'cd mit_6828_os_labs && make grade'
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
